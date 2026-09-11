#ifndef SANKATNET_BACKEND_PUBLISHER_H
#define SANKATNET_BACKEND_PUBLISHER_H

#include <Arduino.h>

#include "../config/GatewayConfig.h"

#include "../network/Packet.h"
#include "../network/PacketCodec.h"

#include "MQTTManager.h"


class BackendPublisher
{
public:

    BackendPublisher()
        : _mqtt(nullptr),
          _initialized(false),
          _published(0),
          _failed(0)
    {
    }


    /*
     * ========================================================
     * BEGIN
     * ========================================================
     */

    void begin(MQTTManager *mqtt)
    {
        _mqtt = mqtt;

        _initialized =
            (_mqtt != nullptr);
    }


    /*
     * ========================================================
     * GENERIC PACKET PUBLISHER
     * ========================================================
     */

    bool publish(PacketContainer &packet)
    {
        if (!_initialized ||
            _mqtt == nullptr)
        {
            return false;
        }


        if (!_mqtt->isConnected())
        {
            return false;
        }


        switch (packet.header.packetType)
        {
            case PKT_TELEMETRY:
                return publishTelemetry(packet);

            case PKT_ALERT:
                return publishAlert(packet, false);

            case PKT_HEARTBEAT:
                return publishHeartbeat(packet);

            case PKT_ROUTE:
                return publishRoute(packet);

            case PKT_DISCOVERY:
                return publishDiscovery(packet);

            case PKT_CONFIG:
                return publishConfig(packet);

            default:
                Serial.println(
                    "[BACKEND] Unsupported packet type"
                );

                _failed++;

                return false;
        }
    }


    /*
     * ========================================================
     * TELEMETRY
     * ========================================================
     */

    bool publishTelemetry(PacketContainer &packet)
    {
        SensorPayload data;


        if (!PacketCodec::decodeSensorPayload(
                packet,
                data))
        {
            _failed++;

            return false;
        }


        String nodeId =
            makeNodeId(
                packet.header.sourceNode
            );


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_TELEMETRY
            );

        topic += "/";

        topic += nodeId;


        String payload;

        payload.reserve(1100);


        payload += "{";

        appendUInt(
            payload,
            "node_id",
            packet.header.sourceNode
        );

        appendUInt(
            payload,
            "sequence",
            packet.header.sequence
        );

        appendUInt(
            payload,
            "timestamp",
            packet.header.timestamp
        );

        appendFloat(
            payload,
            "temperature",
            data.temperature_x100 / 100.0f
        );

        appendFloat(
            payload,
            "humidity",
            data.humidity_x100 / 100.0f
        );

        appendUInt(
            payload,
            "pressure_pa",
            data.pressure_pa
        );

        appendFloat(
            payload,
            "latitude",
            data.latitude_x1e6 / 1000000.0f
        );

        appendFloat(
            payload,
            "longitude",
            data.longitude_x1e6 / 1000000.0f
        );

        appendInt(
            payload,
            "altitude_m",
            data.altitude_m
        );

        appendUInt(
            payload,
            "mq2",
            data.mq2
        );

        appendUInt(
            payload,
            "mq135",
            data.mq135
        );

        appendUInt(
            payload,
            "water_level",
            data.waterLevel
        );

        appendUInt(
            payload,
            "soil_moisture",
            data.soilMoisture
        );

        appendFloat(
            payload,
            "soil_temperature",
            data.soilTemperature_x100 / 100.0f
        );

        appendFloat(
            payload,
            "vibration",
            data.vibration_x100 / 100.0f
        );

        appendFloat(
            payload,
            "rainfall_mm",
            data.rainfall_x10 / 10.0f
        );

        appendFloat(
            payload,
            "wind_speed",
            data.windSpeed_x10 / 10.0f
        );

        appendUInt(
            payload,
            "turbidity",
            data.turbidity
        );

        appendFloat(
            payload,
            "ph",
            data.ph_x100 / 100.0f
        );

        appendUInt(
            payload,
            "tds",
            data.tds
        );

        appendUInt(
            payload,
            "battery_mv",
            data.battery_mV
        );

        appendUInt(
            payload,
            "risk_score",
            data.riskScore
        );

        appendUInt(
            payload,
            "hazard",
            data.hazardType
        );

        appendUInt(
            payload,
            "sensor_status",
            data.sensorStatus,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_TELEMETRY,
            GatewayConfig::MQTT_RETAIN_TELEMETRY
        );
    }


    /*
     * ========================================================
     * ALERT
     * ========================================================
     */

    bool publishAlert(
        PacketContainer &packet,
        bool critical)
    {
        AlertPayload alert;


        if (!PacketCodec::decodeAlertPayload(
                packet,
                alert))
        {
            _failed++;

            return false;
        }


        String nodeId =
            makeNodeId(
                packet.header.sourceNode
            );


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_ALERTS
            );

        topic += "/";

        topic += nodeId;


        String payload;

        payload.reserve(700);


        payload += "{";

        appendUInt(
            payload,
            "node_id",
            packet.header.sourceNode
        );

        appendUInt(
            payload,
            "sequence",
            packet.header.sequence
        );

        appendUInt(
            payload,
            "timestamp",
            packet.header.timestamp
        );

        appendUInt(
            payload,
            "hazard",
            alert.hazardType
        );

        appendUInt(
            payload,
            "priority",
            alert.priority
        );

        appendUInt(
            payload,
            "risk_score",
            alert.riskScore
        );

        appendUInt(
            payload,
            "confidence",
            alert.confidence
        );

        appendUInt(
            payload,
            "critical",
            alert.critical
        );

        appendUInt(
            payload,
            "sensor_status",
            alert.sensorStatus
        );

        appendUInt(
            payload,
            "detected_at",
            alert.detectedAt,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_ALERT,
            GatewayConfig::MQTT_RETAIN_ALERT
        );
    }


    /*
     * ========================================================
     * HEARTBEAT
     * ========================================================
     */

    bool publishHeartbeat(PacketContainer &packet)
    {
        HeartbeatPayload heartbeat;


        if (!PacketCodec::decodeHeartbeatPayload(
                packet,
                heartbeat))
        {
            _failed++;

            return false;
        }


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_HEARTBEAT
            );

        topic += "/";

        topic += makeNodeId(
            heartbeat.nodeId
        );


        String payload;

        payload.reserve(500);


        payload += "{";

        appendUInt(
            payload,
            "node_id",
            heartbeat.nodeId
        );

        appendUInt(
            payload,
            "role",
            heartbeat.nodeRole
        );

        appendUInt(
            payload,
            "battery_state",
            heartbeat.batteryState
        );

        appendUInt(
            payload,
            "battery_mv",
            heartbeat.battery_mV
        );

        appendUInt(
            payload,
            "risk_score",
            heartbeat.riskScore
        );

        appendUInt(
            payload,
            "hazard",
            heartbeat.hazardType
        );

        appendUInt(
            payload,
            "sensor_status",
            heartbeat.sensorStatus
        );

        appendUInt(
            payload,
            "uptime_seconds",
            heartbeat.uptimeSeconds,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_HEARTBEAT,
            GatewayConfig::MQTT_RETAIN_HEARTBEAT
        );
    }


    /*
     * ========================================================
     * ROUTE
     * ========================================================
     */

    bool publishRoute(PacketContainer &packet)
    {
        RoutePayload route;


        if (!PacketCodec::decodeRoutePayload(
                packet,
                route))
        {
            _failed++;

            return false;
        }


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_ROUTE
            );


        String payload;

        payload.reserve(600);


        payload += "{";

        appendUInt(
            payload,
            "source_node",
            packet.header.sourceNode
        );

        appendUInt(
            payload,
            "sequence",
            packet.header.sequence
        );

        appendUInt(
            payload,
            "message_type",
            route.routeMessageType
        );

        appendUInt(
            payload,
            "destination",
            route.destinationNode
        );

        appendUInt(
            payload,
            "next_hop",
            route.nextHop
        );

        appendUInt(
            payload,
            "hop_count",
            route.hopCount
        );

        appendInt(
            payload,
            "rssi",
            route.rssi
        );

        appendFloat(
            payload,
            "snr",
            route.snr_x10 / 10.0f
        );

        appendUInt(
            payload,
            "timestamp",
            route.timestamp,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_GATEWAY,
            false
        );
    }


    /*
     * ========================================================
     * DISCOVERY
     * ========================================================
     */

    bool publishDiscovery(PacketContainer &packet)
    {
        DiscoveryPayload discovery;


        if (!PacketCodec::decodeDiscoveryPayload(
                packet,
                discovery))
        {
            _failed++;

            return false;
        }


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_DISCOVERY
            );

        topic += "/";

        topic += makeNodeId(
            discovery.nodeId
        );


        String payload;

        payload.reserve(500);


        payload += "{";

        appendUInt(
            payload,
            "node_id",
            discovery.nodeId
        );

        appendUInt(
            payload,
            "role",
            discovery.nodeRole
        );

        appendUInt(
            payload,
            "capabilities",
            discovery.capabilities
        );

        appendUInt(
            payload,
            "max_tx_power",
            discovery.maxTxPower
        );

        appendUInt(
            payload,
            "timestamp",
            discovery.timestamp,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_GATEWAY,
            false
        );
    }


    /*
     * ========================================================
     * CONFIGURATION
     * ========================================================
     */

    bool publishConfig(PacketContainer &packet)
    {
        ConfigPayload config;


        if (!PacketCodec::decodeConfigPayload(
                packet,
                config))
        {
            _failed++;

            return false;
        }


        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_CONFIG
            );


        String payload;

        payload.reserve(500);


        payload += "{";

        appendUInt(
            payload,
            "source_node",
            packet.header.sourceNode
        );

        appendUInt(
            payload,
            "target_node",
            config.targetNode
        );

        appendUInt(
            payload,
            "configuration_version",
            config.configurationVersion
        );

        appendUInt(
            payload,
            "telemetry_interval_ms",
            config.telemetryIntervalMs
        );

        appendUInt(
            payload,
            "heartbeat_interval_ms",
            config.heartbeatIntervalMs
        );

        appendUInt(
            payload,
            "timestamp",
            config.timestamp,
            true
        );


        return mqttPublish(
            topic,
            payload,
            GatewayConfig::MQTT_QOS_GATEWAY,
            false
        );
    }


    /*
     * ========================================================
     * GATEWAY HEARTBEAT
     * ========================================================
     */

    bool publishGatewayHeartbeat(
        uint16_t gatewayId,
        uint32_t received,
        uint32_t forwarded,
        uint32_t published,
        uint32_t dropped,
        uint16_t queueSize)
    {
        if (!_initialized ||
            _mqtt == nullptr ||
            !_mqtt->isConnected())
        {
            return false;
        }


        String payload;

        payload.reserve(500);


        payload += "{";

        appendUInt(
            payload,
            "gateway_id",
            gatewayId
        );

        appendUInt(
            payload,
            "received_packets",
            received
        );

        appendUInt(
            payload,
            "forwarded_packets",
            forwarded
        );

        appendUInt(
            payload,
            "published_packets",
            published
        );

        appendUInt(
            payload,
            "dropped_packets",
            dropped
        );

        appendUInt(
            payload,
            "queue_size",
            queueSize,
            true
        );


        return mqttPublish(
            String(
                GatewayConfig::MQTT_TOPIC_GATEWAY_HEALTH
            ),
            payload,
            GatewayConfig::MQTT_QOS_GATEWAY,
            true
        );
    }


    /*
     * ========================================================
     * STATISTICS
     * ========================================================
     */

    uint32_t publishedCount() const
    {
        return _published;
    }


    uint32_t failedCount() const
    {
        return _failed;
    }


private:

    MQTTManager *_mqtt;

    bool _initialized;

    uint32_t _published;

    uint32_t _failed;


    /*
     * ========================================================
     * MQTT PUBLISH HELPER
     * ========================================================
     */

    bool mqttPublish(
        const String &topic,
        const String &payload,
        uint8_t qos,
        bool retained)
    {
        if (!_mqtt ||
            !_mqtt->isConnected())
        {
            _failed++;

            return false;
        }


        if (payload.length() >
            GatewayConfig::MQTT_MAX_PAYLOAD_SIZE)
        {
            Serial.println(
                "[BACKEND] MQTT payload too large"
            );

            _failed++;

            return false;
        }


        bool result =
            _mqtt->publish(
                topic.c_str(),
                payload,
                qos,
                retained
            );


        if (result)
        {
            _published++;
        }
        else
        {
            _failed++;
        }


        return result;
    }


    /*
     * ========================================================
     * NODE ID
     * ========================================================
     */

    String makeNodeId(
        uint16_t nodeId)
    {
        String result = "NODE-";

        if (nodeId < 10)
        {
            result += "0";
        }

        result += String(nodeId);

        return result;
    }


    /*
     * ========================================================
     * JSON HELPERS
     * ========================================================
     *
     * These helpers generate backend JSON.
     *
     * JSON is used ONLY on the Internet side.
     *
     * LoRa remains binary.
     * ========================================================
     */

    void appendUInt(
        String &json,
        const char *key,
        uint32_t value,
        bool last = false)
    {
        json += "\"";
        json += key;
        json += "\":";
        json += String(value);

        if (!last)
        {
            json += ",";
        }
    }


    void appendInt(
        String &json,
        const char *key,
        int32_t value,
        bool last = false)
    {
        json += "\"";
        json += key;
        json += "\":";
        json += String(value);

        if (!last)
        {
            json += ",";
        }
    }


    void appendFloat(
        String &json,
        const char *key,
        float value,
        bool last = false)
    {
        json += "\"";
        json += key;
        json += "\":";
        json += String(value, 4);

        if (!last)
        {
            json += ",";
        }
    }
};


#endif