/*
 * ============================================================
 * Sankat-Net Gateway
 * ============================================================
 *
 * Role:
 *   LoRa Mesh Gateway + Internet Uplink
 *
 * Data Flow:
 *
 *   Field Node
 *       |
 *       | SX1278 LoRa
 *       v
 *   LoRa Mesh
 *       |
 *       v
 *   Gateway ESP32
 *       |
 *       +--> Validate Packet
 *       +--> AES-256-GCM Authentication/Decryption
 *       +--> Replay Protection
 *       +--> Packet Decode
 *       +--> Mesh Routing / ACK
 *       |
 *       +--> MQTT over TLS
 *                |
 *                v
 *           MQTT Broker
 *                |
 *                v
 *             Backend
 *
 * ============================================================
 */

#include <Arduino.h>

#include "../config/GatewayConfig.h"
#include "../config/PinConfig.h"
#include "../config/SecurityConfig.h"

#include "../network/LoRaDriver.h"
#include "../network/Packet.h"
#include "../network/PacketCodec.h"
#include "../network/CryptoManager.h"
#include "../network/MeshRouter.h"
#include "../network/ReplayProtection.h"
#include "../../network/AckManager.h"

#include "../internet/WiFiManager.h"
#include "../internet/MQTTManager.h"
#include "../internet/BackendPublisher.h"

#include "../storage/GatewayQueue.h"

#include "../system/Watchdog.h"
#include "../system/GatewayHealth.h"

using namespace SankatNet;


/*
 * ------------------------------------------------------------
 * Global module instances
 * ------------------------------------------------------------
 */

LoRaDriver        loraDriver;
CryptoManager     cryptoManager;
MeshRouter        meshRouter;
ReplayProtection  replayProtection;
AckManager        ackManager;

WiFiManager       wifiManager;
MQTTManager       mqttManager;
BackendPublisher  backendPublisher;

GatewayQueue      gatewayQueue;

Watchdog           watchdog;
GatewayHealth      gatewayHealth;


/*
 * ------------------------------------------------------------
 * Runtime state
 * ------------------------------------------------------------
 */

static uint32_t lastHeartbeat       = 0;
static uint32_t lastMQTTCheck       = 0;
static uint32_t lastQueueProcess    = 0;
static uint32_t lastHealthCheck     = 0;

static uint32_t receivedPackets     = 0;
static uint32_t forwardedPackets    = 0;
static uint32_t publishedPackets    = 0;
static uint32_t droppedPackets      = 0;


/*
 * ------------------------------------------------------------
 * LoRa receive buffer
 * ------------------------------------------------------------
 */

static uint8_t loraRxBuffer[
    GatewayConfig::MAX_LORA_PACKET_SIZE
];


/*
 * ------------------------------------------------------------
 * Forward declarations
 * ------------------------------------------------------------
 */

void processLoRaPacket();

void processPacket(PacketContainer &packet);

void handleTelemetry(PacketContainer &packet);

void handleAlert(PacketContainer &packet);

void handleHeartbeat(PacketContainer &packet);

void handleRoute(PacketContainer &packet);

void handleDiscovery(PacketContainer &packet);

void handleConfig(PacketContainer &packet);

void handleAck(PacketContainer &packet);

void publishPacketToBackend(PacketContainer &packet);

void processGatewayQueue();

void maintainInternetConnection();

void sendGatewayHeartbeat();

void updateGatewayHealth();


/*
 * ============================================================
 * SETUP
 * ============================================================
 */

void setup()
{
    /*
     * --------------------------------------------------------
     * Serial
     * --------------------------------------------------------
     */

    Serial.begin(Pins::SERIAL_BAUDRATE);

    delay(300);

    Serial.println();
    Serial.println("==========================================");
    Serial.println("       SANKAT-NET GATEWAY STARTING");
    Serial.println("==========================================");


    /*
     * --------------------------------------------------------
     * Watchdog
     * --------------------------------------------------------
     */

    if (!watchdog.begin())
    {
        Serial.println("[WARN] Watchdog initialization failed");
    }


    /*
     * --------------------------------------------------------
     * Gateway health
     * --------------------------------------------------------
     */

    gatewayHealth.begin();


    /*
     * --------------------------------------------------------
     * LoRa
     * --------------------------------------------------------
     */

    Serial.println("[INIT] Initializing SX1278...");

    if (!loraDriver.begin())
    {
        Serial.println("[ERROR] LoRa initialization failed");
        gatewayHealth.raiseLoRaFault();
    }
    else
    {
        Serial.println("[OK] LoRa initialized");
        gatewayHealth.clearLoRaFault();
    }


    /*
     * --------------------------------------------------------
     * Crypto
     * --------------------------------------------------------
     */

    Serial.println("[INIT] Initializing security...");

    if (!cryptoManager.begin())
    {
        Serial.println("[ERROR] Crypto initialization failed");
        gatewayHealth.raiseSecurityFault();
    }
    else
    {
        Serial.println("[OK] AES-256-GCM security ready");
        gatewayHealth.clearSecurityFault();
    }


    /*
     * --------------------------------------------------------
     * Replay protection
     * --------------------------------------------------------
     */

    replayProtection.begin();


    /*
     * --------------------------------------------------------
     * Mesh router
     * --------------------------------------------------------
     */

    meshRouter.begin(&loraDriver);


    /*
     * --------------------------------------------------------
     * ACK manager
     * --------------------------------------------------------
     */

    ackManager.begin(&loraDriver);


    /*
     * --------------------------------------------------------
     * Gateway queue
     * --------------------------------------------------------
     *
     * Used when Internet connectivity disappears.
     *
     * LoRa packets can continue arriving even when Wi-Fi/
     * MQTT is unavailable.
     *
     * --------------------------------------------------------
     */

    gatewayQueue.begin();


    /*
     * --------------------------------------------------------
     * Wi-Fi
     * --------------------------------------------------------
     */

    Serial.println("[INIT] Connecting to Wi-Fi...");

    if (wifiManager.begin())
    {
        Serial.println("[OK] Wi-Fi connected");
        gatewayHealth.clearWiFiFault();
    }
    else
    {
        Serial.println(
            "[WARN] Wi-Fi unavailable - "
            "gateway will operate in offline mode"
        );

        gatewayHealth.raiseWiFiFault();
    }


    /*
     * --------------------------------------------------------
     * MQTT
     * --------------------------------------------------------
     */

    if (wifiManager.isConnected())
    {
        Serial.println("[INIT] Starting MQTT...");

        if (mqttManager.begin())
        {
            Serial.println("[OK] MQTT initialized");
            gatewayHealth.clearMQTTFault();
        }
        else
        {
            Serial.println("[WARN] MQTT initialization failed");
            gatewayHealth.raiseMQTTFault();
        }
    }


    /*
     * --------------------------------------------------------
     * Backend publisher
     * --------------------------------------------------------
     */

    backendPublisher.begin(&mqttManager);


    /*
     * --------------------------------------------------------
     * Start LoRa receive mode
     * --------------------------------------------------------
     */

    loraDriver.startReceive();


    /*
     * --------------------------------------------------------
     * Initial timestamps
     * --------------------------------------------------------
     */

    lastHeartbeat    = millis();
    lastMQTTCheck    = millis();
    lastQueueProcess = millis();
    lastHealthCheck  = millis();


    Serial.println("------------------------------------------");
    Serial.println("Gateway ID : 0xFFFF");
    Serial.println("LoRa       : SX1278");
    Serial.println("Uplink     : MQTT over TLS");
    Serial.println("Mode       : LoRa Mesh Gateway");
    Serial.println("------------------------------------------");

    Serial.println("[READY] Sankat-Net Gateway operational");
}


/*
 * ============================================================
 * MAIN LOOP
 * ============================================================
 */

void loop()
{
    watchdog.feed();


    /*
     * --------------------------------------------------------
     * 1. Receive LoRa packets
     * --------------------------------------------------------
     */

    processLoRaPacket();


    /*
     * --------------------------------------------------------
     * 2. Maintain Internet connection
     * --------------------------------------------------------
     */

    maintainInternetConnection();


    /*
     * --------------------------------------------------------
     * 3. Process packets waiting for MQTT upload
     * --------------------------------------------------------
     */

    if (millis() - lastQueueProcess >=
        GatewayConfig::QUEUE_PROCESS_INTERVAL_MS)
    {
        lastQueueProcess = millis();

        processGatewayQueue();
    }


    /*
     * --------------------------------------------------------
     * 4. MQTT processing
     * --------------------------------------------------------
     */

    mqttManager.loop();


    /*
     * --------------------------------------------------------
     * 5. ACK manager
     * --------------------------------------------------------
     */

    ackManager.update();


    /*
     * --------------------------------------------------------
     * 6. Route maintenance
     * --------------------------------------------------------
     */

    meshRouter.expireRoutes();


    /*
     * --------------------------------------------------------
     * 7. Gateway heartbeat
     * --------------------------------------------------------
     */

    if (millis() - lastHeartbeat >=
        GatewayConfig::GATEWAY_HEARTBEAT_INTERVAL_MS)
    {
        lastHeartbeat = millis();

        sendGatewayHeartbeat();
    }


    /*
     * --------------------------------------------------------
     * 8. Health monitoring
     * --------------------------------------------------------
     */

    if (millis() - lastHealthCheck >=
        GatewayConfig::HEALTH_CHECK_INTERVAL_MS)
    {
        lastHealthCheck = millis();

        updateGatewayHealth();
    }


    watchdog.feed();

    delay(2);
}


/*
 * ============================================================
 * PROCESS LoRa PACKET
 * ============================================================
 */

void processLoRaPacket()
{
    size_t packetLength = 0;


    /*
     * Check whether a LoRa packet is available.
     */

    if (!loraDriver.receive(
            loraRxBuffer,
            sizeof(loraRxBuffer),
            packetLength))
    {
        return;
    }


    receivedPackets++;


    Serial.print("[LORA RX] Length=");
    Serial.println(packetLength);


    /*
     * Decode packet.
     */

    PacketContainer packet;

    if (!PacketCodec::decode(
            loraRxBuffer,
            packetLength,
            packet))
    {
        Serial.println("[DROP] Invalid packet encoding");

        droppedPackets++;

        return;
    }


    /*
     * Validate packet header.
     */

    if (!PacketCodec::validateHeader(packet.header) ||
        !PacketCodec::validatePayloadType(packet))
    {
        Serial.println("[DROP] Invalid packet header");

        droppedPackets++;

        return;
    }


    /*
     * --------------------------------------------------------
     * Security verification
     * --------------------------------------------------------
     *
     * Authentication MUST happen before replay-cache insertion.
     *
     * --------------------------------------------------------
     */

    if (::SecurityConfig::SECURITY_REQUIRED)
    {
        if (!isEncrypted(packet.header))
        {
            Serial.println(
                "[SECURITY] Rejected unauthenticated packet"
            );

            droppedPackets++;

            return;
        }


        if (!cryptoManager.authenticatePacket(packet))
        {
            Serial.println(
                "[SECURITY] AES-GCM authentication failed"
            );

            gatewayHealth.recordSecurityFailure();

            droppedPackets++;

            return;
        }
    }


    /*
     * --------------------------------------------------------
     * Replay protection
     * --------------------------------------------------------
     */

    if (!replayProtection.accept(packet.header))
    {
        Serial.println(
            "[SECURITY] Replay / duplicate packet rejected"
        );

        droppedPackets++;

        return;
    }


    /*
     * --------------------------------------------------------
     * Packet processing
     * --------------------------------------------------------
     */

    processPacket(packet);
}


/*
 * ============================================================
 * PROCESS VALIDATED PACKET
 * ============================================================
 */

void processPacket(PacketContainer &packet)
{
    /*
     * Ignore packets generated by this gateway.
     */

    if (packet.header.sourceNode == GatewayConfig::GATEWAY_ID)
    {
        return;
    }


    /*
     * --------------------------------------------------------
     * Destination handling
     * --------------------------------------------------------
     */

    bool addressedToGateway =
        (
            packet.header.destinationNode ==
            GatewayConfig::GATEWAY_ID
        )
        ||
        (
            packet.header.destinationNode ==
            GatewayConfig::BROADCAST_ID
        );


    /*
     * --------------------------------------------------------
     * Mesh forwarding
     * --------------------------------------------------------
     *
     * The gateway remains a mesh participant.
     *
     * Packets that are not intended for the gateway can be
     * forwarded according to the mesh routing table.
     * --------------------------------------------------------
     */

    if (!addressedToGateway)
    {
        if (packet.header.ttl <= 1)
        {
            Serial.println("[MESH] TTL expired");

            droppedPackets++;

            return;
        }


        packet.header.ttl--;
        packet.header.hopCount++;

        if (meshRouter.forward(packet))
        {
            forwardedPackets++;

            Serial.print("[MESH] Forwarded packet from node ");
            Serial.println(packet.header.sourceNode);
        }
        else
        {
            Serial.println("[MESH] Forwarding failed");
        }


        return;
    }

    if (isEncrypted(packet.header) &&
        !cryptoManager.decryptPacket(packet))
    {
        Serial.println("[SECURITY] Local packet decryption failed");
        gatewayHealth.recordSecurityFailure();
        droppedPackets++;
        return;
    }

    if (!PacketCodec::validatePayloadType(packet))
    {
        Serial.println("[DROP] Invalid decrypted payload");
        droppedPackets++;
        return;
    }


    /*
     * --------------------------------------------------------
     * Gateway-destined packet
     * --------------------------------------------------------
     */

    switch (packet.header.packetType)
    {
        case PKT_TELEMETRY:
            handleTelemetry(packet);
            break;


        case PKT_ALERT:
            handleAlert(packet);
            break;


        case PKT_ACK:
            handleAck(packet);
            break;


        case PKT_HEARTBEAT:
            handleHeartbeat(packet);
            break;


        case PKT_ROUTE:
            handleRoute(packet);
            break;


        case PKT_DISCOVERY:
            handleDiscovery(packet);
            break;


        case PKT_CONFIG:
            handleConfig(packet);
            break;


        default:
            Serial.println(
                "[GATEWAY] Unknown packet type"
            );

            droppedPackets++;
            break;
    }
}


/*
 * ============================================================
 * TELEMETRY
 * ============================================================
 */

void handleTelemetry(PacketContainer &packet)
{
    SensorPayload sensorData;


    if (!PacketCodec::decodeSensorPayload(
            packet,
            sensorData))
    {
        Serial.println(
            "[TELEMETRY] Invalid payload"
        );

        droppedPackets++;

        return;
    }


    Serial.print(
        "[TELEMETRY] Node "
    );

    Serial.print(packet.header.sourceNode);

    Serial.print(" Risk=");

    Serial.print(sensorData.riskScore);

    Serial.print(" Hazard=");

    Serial.println(sensorData.hazardType);


    /*
     * Publish to backend.
     *
     * If MQTT is unavailable, BackendPublisher will place
     * the packet into GatewayQueue.
     */

    publishPacketToBackend(packet);
}


/*
 * ============================================================
 * ALERT
 * ============================================================
 */

void handleAlert(PacketContainer &packet)
{
    AlertPayload alert;


    if (!PacketCodec::decodeAlertPayload(
            packet,
            alert))
    {
        Serial.println(
            "[ALERT] Invalid alert payload"
        );

        droppedPackets++;

        return;
    }


    Serial.println();
    Serial.println("==========================================");
    Serial.println("          !!! HAZARD ALERT !!!");
    Serial.println("==========================================");

    Serial.print("Source Node : ");
    Serial.println(packet.header.sourceNode);

    Serial.print("Hazard      : ");
    Serial.println(alert.hazardType);

    Serial.print("Priority    : ");
    Serial.println(alert.priority);

    Serial.print("Risk Score  : ");
    Serial.println(alert.riskScore);

    Serial.print("Confidence  : ");
    Serial.println(alert.confidence);

    Serial.print("Critical    : ");
    Serial.println(alert.critical);

    Serial.println("==========================================");


    /*
     * Critical alerts should be published immediately.
     */

    if (alert.priority == PRIORITY_CRITICAL)
    {
        backendPublisher.publishAlert(
            packet,
            true
        );
    }
    else
    {
        publishPacketToBackend(packet);
    }
}


/*
 * ============================================================
 * HEARTBEAT
 * ============================================================
 */

void handleHeartbeat(PacketContainer &packet)
{
    HeartbeatPayload heartbeat;


    if (!PacketCodec::decodeHeartbeatPayload(
            packet,
            heartbeat))
    {
        Serial.println(
            "[HEARTBEAT] Invalid payload"
        );

        droppedPackets++;

        return;
    }


    Serial.print("[HEARTBEAT] Node ");
    Serial.print(heartbeat.nodeId);

    Serial.print(" Battery=");
    Serial.print(heartbeat.battery_mV);

    Serial.print("mV Risk=");
    Serial.println(heartbeat.riskScore);


    /*
     * Gateway stores node health information through the
     * backend.
     */

    publishPacketToBackend(packet);
}


/*
 * ============================================================
 * ROUTE
 * ============================================================
 */

void handleRoute(PacketContainer &packet)
{
    RoutePayload route;


    if (!PacketCodec::decodeRoutePayload(
            packet,
            route))
    {
        Serial.println(
            "[ROUTE] Invalid route payload"
        );

        droppedPackets++;

        return;
    }


    /*
     * Update gateway routing information.
     */

    meshRouter.updateRoute(
        route.destinationNode,
        route.nextHop,
        route.hopCount,
        route.rssi,
        route.snr_x10
    );


    /*
     * Send route information to backend.
     */

    publishPacketToBackend(packet);
}


/*
 * ============================================================
 * DISCOVERY
 * ============================================================
 */

void handleDiscovery(PacketContainer &packet)
{
    DiscoveryPayload discovery;


    if (!PacketCodec::decodeDiscoveryPayload(
            packet,
            discovery))
    {
        Serial.println(
            "[DISCOVERY] Invalid payload"
        );

        droppedPackets++;

        return;
    }


    Serial.print("[DISCOVERY] Node ");
    Serial.print(discovery.nodeId);

    Serial.print(" Role=");
    Serial.println(discovery.nodeRole);


    publishPacketToBackend(packet);
}


/*
 * ============================================================
 * CONFIGURATION
 * ============================================================
 */

void handleConfig(PacketContainer &packet)
{
    ConfigPayload config;


    if (!PacketCodec::decodeConfigPayload(
            packet,
            config))
    {
        Serial.println(
            "[CONFIG] Invalid payload"
        );

        droppedPackets++;

        return;
    }


    Serial.print("[CONFIG] Target node=");
    Serial.println(config.targetNode);


    /*
     * Configuration packets may later be generated by the
     * backend and delivered through MQTT -> Gateway -> LoRa.
     *
     * This handler currently reports the received configuration.
     */

    publishPacketToBackend(packet);
}


/*
 * ============================================================
 * ACK
 * ============================================================
 */

void handleAck(PacketContainer &packet)
{
    AckPayload ack;


    if (!PacketCodec::decodeAckPayload(
            packet,
            ack))
    {
        Serial.println(
            "[ACK] Invalid payload"
        );

        droppedPackets++;

        return;
    }


    if (!ackManager.processAck(ack))
    {
        Serial.println(
            "[ACK] ACK did not match pending packet"
        );
    }
}


/*
 * ============================================================
 * BACKEND PUBLISH
 * ============================================================
 */

void publishPacketToBackend(PacketContainer &packet)
{
    /*
     * BackendPublisher converts the binary Sankat-Net packet
     * into the backend's MQTT representation.
     *
     * Example:
     *
     * sankatnet/telemetry/NODE-01
     *
     * or
     *
     * sankatnet/alerts/NODE-01
     */

    if (!wifiManager.isConnected() ||
        !mqttManager.isConnected())
    {
        /*
         * Internet unavailable.
         *
         * Store packet locally in gateway queue.
         */

        if (!gatewayQueue.enqueue(packet))
        {
            Serial.println(
                "[QUEUE] Gateway queue full"
            );

            droppedPackets++;
        }
        else
        {
            Serial.println(
                "[QUEUE] Packet stored for later upload"
            );
        }

        return;
    }


    if (backendPublisher.publish(packet))
    {
        publishedPackets++;
    }
    else
    {
        /*
         * MQTT transmission failed.
         *
         * Keep packet for retry.
         */

        gatewayQueue.enqueue(packet);

        Serial.println(
            "[MQTT] Publish failed - packet queued"
        );
    }
}


/*
 * ============================================================
 * PROCESS GATEWAY QUEUE
 * ============================================================
 */

void processGatewayQueue()
{
    if (!wifiManager.isConnected())
    {
        return;
    }


    if (!mqttManager.isConnected())
    {
        return;
    }


    /*
     * Send a limited number of queued packets per cycle.
     *
     * This prevents the gateway from blocking normal LoRa
     * processing when Internet connectivity returns.
     */

    uint8_t processed = 0;


    while (!gatewayQueue.empty() &&
           processed < GatewayConfig::QUEUE_BATCH_SIZE)
    {
        PacketContainer packet;


        if (!gatewayQueue.peek(packet))
        {
            break;
        }


        if (!backendPublisher.publish(packet))
        {
            /*
             * Stop here.
             *
             * Do not remove the packet if publishing failed.
             */

            break;
        }


        gatewayQueue.dequeue(packet);

        publishedPackets++;

        processed++;
    }
}


/*
 * ============================================================
 * INTERNET CONNECTION MANAGEMENT
 * ============================================================
 */

void maintainInternetConnection()
{
    if (millis() - lastMQTTCheck <
        GatewayConfig::NETWORK_CHECK_INTERVAL_MS)
    {
        return;
    }


    lastMQTTCheck = millis();


    /*
     * Wi-Fi connection.
     */

    if (!wifiManager.isConnected())
    {
        if (wifiManager.reconnect())
        {
            Serial.println(
                "[WiFi] Reconnected"
            );

            gatewayHealth.clearWiFiFault();
        }
        else
        {
            gatewayHealth.raiseWiFiFault();

            return;
        }
    }


    /*
     * MQTT connection.
     */

    if (!mqttManager.isConnected())
    {
        if (mqttManager.reconnect())
        {
            Serial.println(
                "[MQTT] Reconnected"
            );

            gatewayHealth.clearMQTTFault();
        }
        else
        {
            gatewayHealth.raiseMQTTFault();
        }
    }
}


/*
 * ============================================================
 * GATEWAY HEARTBEAT
 * ============================================================
 */

void sendGatewayHeartbeat()
{
    if (!wifiManager.isConnected() ||
        !mqttManager.isConnected())
    {
        return;
    }


    backendPublisher.publishGatewayHeartbeat(
        GatewayConfig::GATEWAY_ID,
        receivedPackets,
        forwardedPackets,
        publishedPackets,
        droppedPackets,
        gatewayQueue.size()
    );
}


/*
 * ============================================================
 * HEALTH MONITORING
 * ============================================================
 */

void updateGatewayHealth()
{
    gatewayHealth.update(
        receivedPackets,
        forwardedPackets,
        publishedPackets,
        droppedPackets,
        gatewayQueue.size()
    );


    Serial.print("[HEALTH] LoRa=");
    Serial.print(loraDriver.isInitialized());

    Serial.print(" WiFi=");
    Serial.print(wifiManager.isConnected());

    Serial.print(" MQTT=");
    Serial.print(mqttManager.isConnected());

    Serial.print(" Queue=");
    Serial.println(gatewayQueue.size());
}