#include <Arduino.h>

#include "../config/NodeConfig.h"
#include "../config/PinConfig.h"
#include "../config/SecurityConfig.h"

#include "../sensors/SensorManager.h"

#include "../intelligence/RiskEngine.h"
#include "../intelligence/HazardClassifier.h"

#include "../network/LoRaDriver.h"
#include "../network/Packet.h"
#include "../network/PacketCodec.h"
#include "../network/CryptoManager.h"
#include "../network/MeshRouter.h"
#include "../network/AckManager.h"
#include "../network/ReplayProtection.h"

#include "../storage/NVSManager.h"
#include "../storage/PacketQueue.h"

#include "../system/Watchdog.h"
#include "../system/FaultManager.h"
#include "../system/BatteryManager.h"


using namespace SankatNet;


// ============================================================
// GLOBAL MODULE OBJECTS
// ============================================================

SensorManager       sensorManager;
RiskEngine          riskEngine;
HazardClassifier    hazardClassifier;

LoRaDriver          loRa;
CryptoManager       cryptoManager;
MeshRouter          meshRouter;
AckManager          ackManager;
ReplayProtection    replayProtection;

NVSManager          nvsManager;
PacketQueue         packetQueue;

Watchdog             watchdog;
FaultManager        faultManager;
BatteryManager      batteryManager;


// ============================================================
// RUNTIME STATE
// ============================================================

SensorData sensorData;

RiskResult riskResult;

ClassificationResult hazardResult;

uint32_t lastTelemetryTime = 0;
uint32_t lastHeartbeatTime = 0;
uint32_t lastRouteUpdateTime = 0;
uint32_t lastQueueProcessTime = 0;
uint32_t lastAlertTime = 0;

bool systemReady = false;


// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void initializeSystem();

void updateSensors();

void processLocalIntelligence();

void createTelemetryPacket();

void createAlertPacket();

void createHeartbeatPacket();

void transmitQueuedPackets();

void receiveLoRaPackets();

void processReceivedPacket(
    uint8_t* rawData,
    size_t length
);

void sendPacket(
    PacketContainer& packet
);

void handlePacket(
    PacketContainer& packet
);

void handleTelemetry(
    PacketContainer& packet
);

void handleAlert(
    PacketContainer& packet
);

void handleAck(
    PacketContainer& packet
);

void handleHeartbeat(
    PacketContainer& packet
);

void handleDiscovery(
    PacketContainer& packet
);

void handleConfig(
    PacketContainer& packet
);

void updateSystemHealth();

void printSystemStatus();

uint32_t allocateSequence();

void processAckRetry();


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(
        Pins::SERIAL_BAUDRATE
    );

    delay(500);


    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "        SANKAT-NET ENVIRONMENT NODE"
    );

    Serial.println(
        "========================================"
    );


    initializeSystem();
}

uint32_t allocateSequence()
{
    if (!nvsManager.isInitialized()) {
        faultManager.raise(FaultType::CONFIGURATION);
        return UINT32_MAX;
    }

    uint32_t sequence = nvsManager.nextSequence();

    if (sequence == UINT32_MAX) {
        faultManager.raise(FaultType::CONFIGURATION);
    }

    return sequence;
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    /*
     * Watchdog must be fed regularly.
     */

    watchdog.feed();


    /*
     * Continuous GPS / weather / motion processing.
     */

    sensorManager.update();

    batteryManager.update();


    /*
     * Receive radio packets before doing
     * lower-priority work.
     */

    receiveLoRaPackets();


    /*
     * Local sensor processing.
     */

    updateSensors();


    /*
     * Local hazard intelligence.
     */

    processLocalIntelligence();


    /*
     * Process ACK timeout/retry state.
     */

    ackManager.update();

    processAckRetry();


    /*
     * Remove stale queued packets.
     */

    packetQueue.cleanup();


    /*
     * Forward / transmit queued packets.
     */

    transmitQueuedPackets();


    /*
     * Route maintenance.
     */

    uint32_t now = millis();

    if (
        now - lastRouteUpdateTime >=
        Timing::ROUTE_UPDATE_INTERVAL_MS
    ) {

        meshRouter.expireRoutes();

        lastRouteUpdateTime =
            now;
    }


    /*
     * Periodic telemetry.
     */

    if (
        now - lastTelemetryTime >=
        Timing::TELEMETRY_INTERVAL_MS
    ) {

        createTelemetryPacket();

        lastTelemetryTime =
            now;
    }


    /*
     * Periodic heartbeat.
     */

    if (
        now - lastHeartbeatTime >=
        Timing::HEARTBEAT_INTERVAL_MS
    ) {

        createHeartbeatPacket();

        lastHeartbeatTime =
            now;
    }


    /*
     * System health.
     */

    updateSystemHealth();


    /*
     * Small cooperative delay.
     */

    delay(2);
}


// ============================================================
// SYSTEM INITIALIZATION
// ============================================================

void initializeSystem()
{
    Serial.println(
        "[INIT] Starting Sankat-Net..."
    );


    // --------------------------------------------------------
    // WATCHDOG
    // --------------------------------------------------------

    if (
        !watchdog.begin()
    ) {

        Serial.println(
            "[ERROR] Watchdog initialization failed"
        );

        faultManager.raise(
            FaultType::WATCHDOG
        );
    }
    else {

        Serial.println(
            "[OK] Watchdog initialized"
        );
    }


    // --------------------------------------------------------
    // NVS
    // --------------------------------------------------------

    if (
        !nvsManager.begin()
    ) {

        Serial.println(
            "[ERROR] NVS initialization failed"
        );

        faultManager.raise(
            FaultType::CONFIGURATION
        );
    }
    else {

        Serial.println(
            "[OK] NVS initialized"
        );

        Serial.print(
            "[NVS] Boot count: "
        );

        Serial.println(
            nvsManager.getBootCounter()
        );
    }


    // --------------------------------------------------------
    // SENSOR MANAGER
    // --------------------------------------------------------

    if (
        !sensorManager.begin()
    ) {

        Serial.println(
            "[WARN] Sensor manager initialization incomplete"
        );

        faultManager.raise(
            FaultType::SENSOR_DATA
        );
    }
    else {

        Serial.println(
            "[OK] Sensor manager initialized"
        );
    }


    // --------------------------------------------------------
    // BATTERY
    // --------------------------------------------------------

    if (
        !batteryManager.begin()
    ) {

        Serial.println(
            "[WARN] Battery manager initialization failed"
        );

        faultManager.raise(
            FaultType::LOW_BATTERY
        );
    }
    else {

        Serial.println(
            "[OK] Battery manager initialized"
        );
    }


    // --------------------------------------------------------
    // LORA
    // --------------------------------------------------------

    if (
        !loRa.begin()
    ) {

        Serial.println(
            "[ERROR] LoRa initialization failed"
        );

        faultManager.raise(
            FaultType::LORA
        );
    }
    else {

        Serial.println(
            "[OK] LoRa initialized"
        );
    }


    // --------------------------------------------------------
    // CRYPTO
    // --------------------------------------------------------

    if (
        !cryptoManager.begin()
    ) {

        Serial.println(
            "[ERROR] Crypto initialization failed"
        );

        faultManager.raise(
            FaultType::SECURITY
        );
    }
    else {

        Serial.println(
            "[OK] AES-256-GCM initialized"
        );
    }


    // --------------------------------------------------------
    // MESH ROUTER
    // --------------------------------------------------------

    if (
        !meshRouter.begin(
            &loRa
        )
    ) {

        Serial.println(
            "[ERROR] Mesh router initialization failed"
        );

        faultManager.raise(
            FaultType::COMMUNICATION
        );
    }
    else {

        Serial.println(
            "[OK] Mesh router initialized"
        );
    }


    // --------------------------------------------------------
    // ACK MANAGER
    // --------------------------------------------------------

    if (
        !ackManager.begin(
            &loRa
        )
    ) {

        Serial.println(
            "[WARN] ACK manager initialization failed"
        );
    }
    else {

        Serial.println(
            "[OK] ACK manager initialized"
        );
    }


    // --------------------------------------------------------
    // PACKET QUEUE
    // --------------------------------------------------------

    packetQueue.begin();

    Serial.println(
        "[OK] Packet queue initialized"
    );


    // --------------------------------------------------------
    // REPLAY PROTECTION
    // --------------------------------------------------------

    replayProtection.begin();

    Serial.println(
        "[OK] Replay protection initialized"
    );


    // --------------------------------------------------------
    // RISK ENGINE
    // --------------------------------------------------------

    riskEngine.begin();

    hazardClassifier.begin();

    Serial.println(
        "[OK] Local intelligence initialized"
    );


    // --------------------------------------------------------
    // STATUS
    // --------------------------------------------------------

    pinMode(
        Pins::STATUS_LED,
        OUTPUT
    );

    pinMode(
        Pins::BUZZER,
        OUTPUT
    );


    digitalWrite(
        Pins::STATUS_LED,
        HIGH
    );


    /*
     * Determine whether the core system can operate.
     */

    systemReady =
        !faultManager.hasFault(
            FaultType::LORA
        )
        &&
        !faultManager.hasFault(
            FaultType::SECURITY
        );


    if (
        systemReady
    ) {

        Serial.println();
        Serial.println(
            "[SYSTEM] Sankat-Net node READY"
        );

        Serial.print(
            "[NODE] ID: 0x"
        );

        Serial.println(
            NODE_ID,
            HEX
        );

        Serial.print(
            "[NODE] Capabilities: 0x"
        );

        Serial.println(
            NODE_CAPABILITIES,
            HEX
        );

        Serial.println(
            "========================================"
        );
    }
    else {

        Serial.println(
            "[SYSTEM] CRITICAL INITIALIZATION FAILURE"
        );

        digitalWrite(
            Pins::STATUS_LED,
            LOW
        );
    }
}


// ============================================================
// SENSOR UPDATE
// ============================================================

void updateSensors()
{
    sensorData =
        sensorManager.getData();


    /*
     * BatteryManager is the authoritative battery measurement
     * for the system battery state.
     */

    uint16_t batteryVoltage =
        batteryManager.getVoltageMV();


    if (
        batteryVoltage > 0
    ) {

        sensorData.battery_mV =
            batteryVoltage;
    }


    /*
     * Sensor health is reported through the status bitmask.
     */

    sensorData.sensorStatus =
        sensorManager.getStatus();


    /*
     * Add battery health information.
     */

    if (
        batteryManager.isLow()
    ) {

        sensorData.sensorStatus |=
            SENSOR_STATUS_LOW_BATTERY;
    }


    if (
        batteryManager.isCritical()
    ) {

        sensorData.sensorStatus |=
            SENSOR_STATUS_CRITICAL_BATTERY;
    }
}


// ============================================================
// LOCAL INTELLIGENCE
// ============================================================

void processLocalIntelligence()
{
    uint32_t now = millis();

    /*
     * Convert current sensor data into a local risk result.
     */

    riskResult =
        riskEngine.evaluate(
            sensorData
        );


    /*
     * Classify the resulting hazard.
     */

    hazardResult =
        hazardClassifier.classify(
            riskResult
        );


    /*
     * Critical local event.
     *
     * Alert is generated immediately instead of waiting for
     * the normal telemetry interval.
     */

    if (
        hazardResult.alertRequired &&
        riskResult.priority ==
            PRIORITY_CRITICAL &&
        now - lastAlertTime >=
            AlertConfig::ALERT_REPEAT_INTERVAL_MS
    ) {

        createAlertPacket();
        lastAlertTime = now;
    }
}


// ============================================================
// CREATE TELEMETRY PACKET
// ============================================================

void createTelemetryPacket()
{
    if (
        !systemReady
    ) {
        return;
    }


    PacketContainer packet;

    memset(
        &packet,
        0,
        sizeof(packet)
    );


    // --------------------------------------------------------
    // HEADER
    // --------------------------------------------------------

    packet.header.magic =
        PACKET_MAGIC;

    packet.header.version =
        PACKET_VERSION;

    packet.header.packetType =
        PKT_TELEMETRY;

    packet.header.flags =
        FLAG_ENCRYPTED |
        FLAG_ACK_REQUIRED;

    packet.header.sourceNode =
        NODE_ID;

    packet.header.destinationNode =
        GATEWAY_ID;

    packet.header.sequence = allocateSequence();

    if (packet.header.sequence == UINT32_MAX) {
        return;
    }

    packet.header.ttl =
        MeshConfig::DEFAULT_TTL;

    packet.header.hopCount =
        0;

    packet.header.timestamp =
        millis();


    // --------------------------------------------------------
    // SENSOR PAYLOAD
    // --------------------------------------------------------

    SensorPayload payload;

    memset(
        &payload,
        0,
        sizeof(payload)
    );


    payload.temperature_x100 =
        static_cast<int16_t>(
            sensorData.temperature * 100.0f
        );


    payload.humidity_x100 =
        static_cast<uint16_t>(
            constrain(
                sensorData.humidity * 100.0f,
                0.0f,
                10000.0f
            )
        );


    payload.pressure_pa =
        static_cast<uint32_t>(
            max(
                0.0f,
                sensorData.pressure
            )
        );


    payload.latitude_x1e6 =
        static_cast<int32_t>(
            sensorData.latitude * 1000000.0
        );


    payload.longitude_x1e6 =
        static_cast<int32_t>(
            sensorData.longitude * 1000000.0
        );


    payload.altitude_m =
        static_cast<int16_t>(
            sensorData.altitude
        );


    payload.mq2 =
        sensorData.mq2;


    payload.mq135 =
        sensorData.mq135;


    payload.waterLevel =
        sensorData.waterLevel;


    payload.soilMoisture =
        sensorData.soilMoisture;


    payload.soilTemperature_x100 =
        static_cast<int16_t>(
            sensorData.soilTemperature * 100.0f
        );


    payload.vibration_x100 =
        static_cast<uint16_t>(
            max(
                0.0f,
                sensorData.vibrationMS2 * 100.0f
            )
        );


    payload.rainfall_x10 =
        static_cast<uint16_t>(
            max(
                0.0f,
                sensorData.rainfallMM * 10.0f
            )
        );


    payload.windSpeed_x10 =
        static_cast<uint16_t>(
            max(
                0.0f,
                sensorData.windSpeedMS * 10.0f
            )
        );


    payload.turbidity =
        sensorData.turbidity;


    payload.ph_x100 =
        static_cast<uint16_t>(
            constrain(
                sensorData.ph * 100.0f,
                0.0f,
                1400.0f
            )
        );


    payload.tds =
        sensorData.tds;


    payload.battery_mV =
        sensorData.battery_mV;


    payload.riskScore =
        riskResult.overallRisk;


    payload.hazardType =
        static_cast<uint8_t>(
            riskResult.hazard
        );


    payload.sensorStatus =
        sensorData.sensorStatus;


    // --------------------------------------------------------
    // COPY PAYLOAD
    // --------------------------------------------------------

    packet.payloadLength =
        sizeof(SensorPayload);


    memcpy(
        packet.payload,
        &payload,
        sizeof(SensorPayload)
    );


    // --------------------------------------------------------
    // ENCRYPT
    // --------------------------------------------------------

    if (
        !cryptoManager.encryptPacket(
            packet
        )
    ) {

        Serial.println(
            "[ERROR] Telemetry encryption failed"
        );

        faultManager.raise(
            FaultType::SECURITY
        );

        return;
    }


    // --------------------------------------------------------
    // QUEUE
    // --------------------------------------------------------

    if (
        !packetQueue.enqueue(
            packet
        )
    ) {

        Serial.println(
            "[WARN] Telemetry queue full"
        );

        faultManager.raise(
            FaultType::PACKET_QUEUE
        );

        return;
    }


    Serial.print(
        "[TX-QUEUE] Telemetry seq="
    );

    Serial.println(
        packet.header.sequence
    );
}


// ============================================================
// CREATE ALERT PACKET
// ============================================================

void createAlertPacket()
{
    if (
        !systemReady
    ) {
        return;
    }


    PacketContainer packet;

    memset(
        &packet,
        0,
        sizeof(packet)
    );


    packet.header.magic =
        PACKET_MAGIC;

    packet.header.version =
        PACKET_VERSION;

    packet.header.packetType =
        PKT_ALERT;

    packet.header.flags =
        FLAG_ENCRYPTED |
        FLAG_ACK_REQUIRED |
        FLAG_CRITICAL |
        FLAG_LOCAL_ALERT;

    packet.header.sourceNode =
        NODE_ID;

    packet.header.destinationNode =
        GATEWAY_ID;

    packet.header.sequence = allocateSequence();

    if (packet.header.sequence == UINT32_MAX) {
        return;
    }

    packet.header.ttl =
        MeshConfig::DEFAULT_TTL;

    packet.header.hopCount =
        0;

    packet.header.timestamp =
        millis();


    AlertPayload alert;

    memset(
        &alert,
        0,
        sizeof(alert)
    );


    /*
     * Populate alert information.
     *
     * Field names must match the AlertPayload definition in
     * Packet.h.
     */

    alert.hazardType =
        static_cast<uint8_t>(
            riskResult.hazard
        );

    alert.priority =
        static_cast<uint8_t>(
            riskResult.priority
        );

    alert.riskScore =
        riskResult.overallRisk;

    alert.fireRisk =
        riskResult.fireRisk;

    alert.floodRisk =
        riskResult.floodRisk;

    alert.seismicRisk =
        riskResult.seismicRisk;

    alert.pollutionRisk =
        riskResult.pollutionRisk;

    alert.confidence =
        hazardResult.confidence;


    packet.payloadLength =
        sizeof(AlertPayload);


    memcpy(
        packet.payload,
        &alert,
        sizeof(AlertPayload)
    );


    if (
        !cryptoManager.encryptPacket(
            packet
        )
    ) {

        Serial.println(
            "[ERROR] ALERT encryption failed"
        );

        faultManager.raise(
            FaultType::SECURITY
        );

        return;
    }


    if (
        !packetQueue.enqueue(
            packet
        )
    ) {

        Serial.println(
            "[ERROR] CRITICAL ALERT could not be queued"
        );

        faultManager.raise(
            FaultType::PACKET_QUEUE
        );

        return;
    }


    /*
     * Local indication.
     */

    digitalWrite(
        Pins::STATUS_LED,
        HIGH
    );

    tone(
        Pins::BUZZER,
        2500,
        200
    );


    Serial.println(
        "[ALERT] CRITICAL HAZARD ALERT QUEUED"
    );
}


// ============================================================
// CREATE HEARTBEAT
// ============================================================

void createHeartbeatPacket()
{
    if (
        !systemReady
    ) {
        return;
    }


    PacketContainer packet;

    memset(
        &packet,
        0,
        sizeof(packet)
    );


    packet.header.magic =
        PACKET_MAGIC;

    packet.header.version =
        PACKET_VERSION;

    packet.header.packetType =
        PKT_HEARTBEAT;

    packet.header.flags =
        FLAG_ENCRYPTED;

    packet.header.sourceNode =
        NODE_ID;

    packet.header.destinationNode =
        GATEWAY_ID;

    packet.header.sequence = allocateSequence();

    if (packet.header.sequence == UINT32_MAX) {
        return;
    }

    packet.header.ttl =
        MeshConfig::DEFAULT_TTL;

    packet.header.hopCount =
        0;

    packet.header.timestamp =
        millis();


    HeartbeatPayload heartbeat;

    memset(
        &heartbeat,
        0,
        sizeof(heartbeat)
    );


    heartbeat.nodeId =
        NODE_ID;

    heartbeat.battery_mV =
        sensorData.battery_mV;

    heartbeat.riskScore =
        riskResult.overallRisk;

    heartbeat.hazardType =
        static_cast<uint8_t>(
            riskResult.hazard
        );

    heartbeat.sensorStatus =
        sensorData.sensorStatus;

    heartbeat.uptimeSeconds =
        millis() / 1000UL;


    packet.payloadLength =
        sizeof(HeartbeatPayload);


    memcpy(
        packet.payload,
        &heartbeat,
        sizeof(HeartbeatPayload)
    );


    if (
        !cryptoManager.encryptPacket(
            packet
        )
    ) {

        Serial.println(
            "[ERROR] Heartbeat encryption failed"
        );

        return;
    }


    packetQueue.enqueue(
        packet
    );
}


// ============================================================
// TRANSMIT QUEUED PACKETS
// ============================================================

void transmitQueuedPackets()
{
    if (
        !systemReady
    ) {
        return;
    }


    /*
     * Do not continuously transmit.
     */

    uint32_t now =
        millis();


    if (
        now - lastQueueProcessTime < 20
    ) {
        return;
    }


    lastQueueProcessTime =
        now;


    if (
        packetQueue.isEmpty()
    ) {
        return;
    }


    PacketContainer packet;

    /*
     * Peek first.
     *
     * We remove it only after successful transmission.
     */

    if (
        !packetQueue.peek(
            packet
        )
    ) {
        return;
    }


    sendPacket(
        packet
    );
}


// ============================================================
// SEND PACKET
// ============================================================

void sendPacket(
    PacketContainer& packet
)
{
    uint8_t encoded[
        MAX_LORA_PACKET_SIZE
    ];


    size_t encodedLength = 0;


    if (
        !PacketCodec::encode(
            packet,
            encoded,
            sizeof(encoded),
            encodedLength
        )
    ) {

        Serial.println(
            "[ERROR] Packet encoding failed"
        );

        faultManager.raise(
            FaultType::PACKET_QUEUE
        );

        return;
    }


    /*
     * Determine the next hop.
     */

    uint16_t nextHop =
        GATEWAY_ID;


    if (
        packet.header.destinationNode !=
        GATEWAY_ID
    ) {

        nextHop = meshRouter.getNextHop(
            packet.header.destinationNode
        );
    }
    else {

        /*
         * For gateway-directed traffic, use the routing table
         * when available.
         */

        nextHop = meshRouter.getNextHop(GATEWAY_ID);
    }


    /*
     * LoRa broadcast when no specific next-hop route exists.
     */

    bool success =
        loRa.transmit(
            encoded,
            encodedLength
        );


    if (
        success
    ) {

        /*
         * Remove successfully transmitted packet.
         */

        packetQueue.dequeue(
            packet
        );


        /*
         * Register ACK only when requested.
         */

        if (
            packet.header.flags &
            FLAG_ACK_REQUIRED
        ) {

            ackManager.registerPacket(
                packet.header.destinationNode,
                packet.header.sequence,
                packet
            );
        }


        faultManager.communicationRecovered();


        Serial.print(
            "[TX] packet type="
        );

        Serial.print(
            packet.header.packetType
        );

        Serial.print(
            " seq="
        );

        Serial.println(
            packet.header.sequence
        );
    }
    else {

        faultManager.communicationFailure();

        Serial.println(
            "[TX] Transmission failed"
        );
    }
}


// ============================================================
// RECEIVE LORA
// ============================================================

void receiveLoRaPackets()
{
    uint8_t buffer[
        MAX_LORA_PACKET_SIZE
    ];


    size_t length =
        sizeof(buffer);


    size_t packetLength = 0;

    if (!loRa.receive(buffer, sizeof(buffer), packetLength)) {
        return;
    }


    if (packetLength == 0) {
        return;
    }


    processReceivedPacket(
        buffer,
        packetLength
    );
}


// ============================================================
// PROCESS RECEIVED PACKET
// ============================================================

void processReceivedPacket(
    uint8_t* rawData,
    size_t length
)
{
    if (
        rawData == nullptr ||
        length == 0
    ) {
        return;
    }


    PacketContainer packet;

    memset(
        &packet,
        0,
        sizeof(packet)
    );


    if (
        !PacketCodec::decode(
            rawData,
            length,
            packet
        )
    ) {

        Serial.println(
            "[RX] Invalid packet"
        );

        return;
    }


    /*
     * Security rule:
     *
     * NEVER apply replay-cache state before authentication.
     *
     * First verify/decrypt, then replay-check.
     */

    if (packet.header.flags & FLAG_ENCRYPTED) {

        if (!cryptoManager.authenticatePacket(packet)) {

            Serial.println(
                "[SECURITY] Authentication failed"
            );

            faultManager.raise(
                FaultType::SECURITY
            );

            return;
        }
    }
    else {

        if (
            SecurityConfig::
            REJECT_UNAUTHENTICATED_PACKETS
        ) {

            Serial.println(
                "[SECURITY] Plaintext packet rejected"
            );

            return;
        }
    }


    /*
     * Replay protection happens AFTER authentication.
     */

    if (
        !replayProtection.accept(
            packet.header
        )
    ) {

        Serial.println(
            "[SECURITY] Replay packet rejected"
        );

        return;
    }


    /*
     * Pass authenticated packet to mesh router.
     */

    bool shouldForward =
        false;


    PacketContainer routedPacket =
        packet;


    if (!PacketCodec::validatePayloadType(packet) ||
        !meshRouter.route(routedPacket, shouldForward)) {

        /*
         * Router rejected packet.
         */

        return;
    }


    /*
     * If this packet belongs to another node,
     * forward it through the mesh.
     */

    if (
        shouldForward
    ) {

        meshRouter.forward(
            routedPacket
        );

        return;
    }

    if ((packet.header.flags & FLAG_ENCRYPTED) &&
        !cryptoManager.decryptPacket(packet)) {
        faultManager.raise(FaultType::SECURITY);
        return;
    }

    if (!PacketCodec::validatePayloadType(packet)) {
        faultManager.raise(FaultType::SECURITY);
        return;
    }


    /*
     * Packet is for this node.
     */

    handlePacket(
        packet
    );
}


// ============================================================
// PACKET DISPATCHER
// ============================================================

void handlePacket(
    PacketContainer& packet
)
{
    switch (
        packet.header.packetType
    ) {

        case PKT_TELEMETRY:

            handleTelemetry(
                packet
            );

            break;


        case PKT_ALERT:

            handleAlert(
                packet
            );

            break;


        case PKT_ACK:

            handleAck(
                packet
            );

            break;


        case PKT_HEARTBEAT:

            handleHeartbeat(
                packet
            );

            break;


        case PKT_DISCOVERY:

            handleDiscovery(
                packet
            );

            break;


        case PKT_CONFIG:

            handleConfig(
                packet
            );

            break;


        case PKT_ROUTE:

            /*
             * Route packets are handled by MeshRouter.
             */

            break;


        default:

            Serial.println(
                "[RX] Unknown packet type"
            );

            break;
    }
}


// ============================================================
// TELEMETRY HANDLER
// ============================================================

void handleTelemetry(
    PacketContainer& packet
)
{
    if (
        packet.payloadLength !=
        sizeof(SensorPayload)
    ) {

        Serial.println(
            "[RX] Invalid telemetry payload"
        );

        return;
    }


    SensorPayload payload;


    memcpy(
        &payload,
        packet.payload,
        sizeof(payload)
    );


    Serial.print(
        "[RX-TELEMETRY] Node 0x"
    );

    Serial.print(
        packet.header.sourceNode,
        HEX
    );

    Serial.print(
        " Risk="
    );

    Serial.print(
        payload.riskScore
    );

    Serial.print(
        " Hazard="
    );

    Serial.println(
        payload.hazardType
    );
}


// ============================================================
// ALERT HANDLER
// ============================================================

void handleAlert(
    PacketContainer& packet
)
{
    if (
        packet.payloadLength !=
        sizeof(AlertPayload)
    ) {

        return;
    }


    AlertPayload alert;


    memcpy(
        &alert,
        packet.payload,
        sizeof(alert)
    );


    Serial.println();
    Serial.println(
        "******** SANKAT-NET ALERT ********"
    );


    Serial.print(
        "Source Node : 0x"
    );

    Serial.println(
        packet.header.sourceNode,
        HEX
    );


    Serial.print(
        "Hazard      : "
    );

    Serial.println(
        alert.hazardType
    );


    Serial.print(
        "Priority    : "
    );

    Serial.println(
        alert.priority
    );


    Serial.print(
        "Risk        : "
    );

    Serial.println(
        alert.riskScore
    );


    Serial.println(
        "***********************************"
    );


    /*
     * Critical local indication.
     */

    if (
        alert.priority >=
        PRIORITY_CRITICAL
    ) {

        tone(
            Pins::BUZZER,
            3000,
            500
        );
    }
}


// ============================================================
// ACK HANDLER
// ============================================================

void handleAck(
    PacketContainer& packet
)
{
    if (
        packet.payloadLength !=
        sizeof(AckPayload)
    ) {

        return;
    }


    AckPayload ack;


    memcpy(
        &ack,
        packet.payload,
        sizeof(ack)
    );


    ackManager.processAck(
        ack
    );


    Serial.println(
        "[ACK] ACK processed"
    );
}


// ============================================================
// HEARTBEAT HANDLER
// ============================================================

void handleHeartbeat(
    PacketContainer& packet
)
{
    if (
        packet.payloadLength !=
        sizeof(HeartbeatPayload)
    ) {

        return;
    }


    HeartbeatPayload heartbeat;


    memcpy(
        &heartbeat,
        packet.payload,
        sizeof(heartbeat)
    );


    Serial.print(
        "[HEARTBEAT] Node 0x"
    );

    Serial.print(
        heartbeat.nodeId,
        HEX
    );

    Serial.print(
        " Battery="
    );

    Serial.print(
        heartbeat.battery_mV
    );

    Serial.print(
        "mV Risk="
    );

    Serial.println(
        heartbeat.riskScore
    );
}


// ============================================================
// DISCOVERY HANDLER
// ============================================================

void handleDiscovery(
    PacketContainer& packet
)
{
    Serial.print(
        "[DISCOVERY] Node 0x"
    );

    Serial.print(
        packet.header.sourceNode,
        HEX
    );

    Serial.println(
        " discovered"
    );
}


// ============================================================
// CONFIG HANDLER
// ============================================================

void handleConfig(
    PacketContainer& packet
)
{
    /*
     * Configuration packets must be authenticated before
     * reaching this function.
     */

    Serial.print(
        "[CONFIG] Configuration packet from node 0x"
    );

    Serial.println(
        packet.header.sourceNode,
        HEX
    );


    /*
     * Actual configuration changes should be implemented
     * only after command authorization is defined.
     *
     * Do not blindly apply received configuration values.
     */
}


// ============================================================
// SYSTEM HEALTH
// ============================================================

void updateSystemHealth()
{
    /*
     * Battery fault handling.
     */

    uint16_t battery =
        batteryManager.getVoltageMV();


    if (
        battery > 0
    ) {

        faultManager.updateBattery(
            battery
        );
    }


    /*
     * Sensor health.
     */

    if (
        !sensorManager.sensorHealthy()
    ) {

        faultManager.raise(
            FaultType::SENSOR_DATA
        );
    }
    else {

        faultManager.clear(
            FaultType::SENSOR_DATA
        );
    }


    /*
     * LoRa health.
     */

    if (
        !loRa.isInitialized()
    ) {

        faultManager.raise(
            FaultType::LORA
        );
    }


    /*
     * Security health.
     */

    if (
        !cryptoManager.isInitialized()
    ) {

        faultManager.raise(
            FaultType::SECURITY
        );
    }


    /*
     * Watchdog health.
     */

    if (
        !watchdog.isInitialized()
    ) {

        faultManager.raise(
            FaultType::WATCHDOG
        );
    }
}


// ============================================================
// DEBUG STATUS
// ============================================================

void printSystemStatus()
{
    Serial.println();
    Serial.println(
        "========== SANKAT-NET STATUS =========="
    );


    Serial.print(
        "Node ID       : 0x"
    );

    Serial.println(
        NODE_ID,
        HEX
    );


    Serial.print(
        "Role          : "
    );

    Serial.println(
        static_cast<uint8_t>(
            NODE_ROLE
        )
    );


    Serial.print(
        "Battery       : "
    );

    Serial.print(
        batteryManager.getVoltageMV()
    );

    Serial.println(
        " mV"
    );


    Serial.print(
        "Risk          : "
    );

    Serial.println(
        riskResult.overallRisk
    );


    Serial.print(
        "Hazard        : "
    );

    Serial.println(
        static_cast<uint8_t>(
            riskResult.hazard
        )
    );


    Serial.print(
        "Queue         : "
    );

    Serial.println(
        packetQueue.size()
    );


    Serial.print(
        "Crypto TX     : "
    );

    Serial.println(
        cryptoManager.getEncryptedPackets()
    );


    Serial.print(
        "Crypto RX     : "
    );

    Serial.println(
        cryptoManager.getDecryptedPackets()
    );


    Serial.print(
        "Auth failures : "
    );

    Serial.println(
        cryptoManager.getAuthenticationFailures()
    );


    Serial.println(
        "========================================"
    );
}

void processAckRetry()
{
    if (!ackManager.retryNeeded()) {
        return;
    }

    PacketContainer packet;

    if (ackManager.getRetryPacket(packet)) {
        uint8_t encoded[MAX_LORA_PACKET_SIZE];
        size_t encodedLength = 0;

        if (PacketCodec::encode(
                packet,
                encoded,
                sizeof(encoded),
                encodedLength) &&
            loRa.transmit(encoded, encodedLength)) {
            faultManager.communicationRecovered();
        }
        else {
            faultManager.communicationFailure();
        }
    }

    ackManager.clearRetryRequest();
}