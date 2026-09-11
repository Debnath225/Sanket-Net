#ifndef SANKATNET_PACKET_H
#define SANKATNET_PACKET_H

#include <Arduino.h>
#include "../config/NodeConfig.h"

namespace SankatNet {

// ============================================================
// Protocol constants
// ============================================================

static constexpr uint16_t PACKET_MAGIC = 0x534E;   // "SN"
static constexpr uint8_t PACKET_VERSION = 1;

// ============================================================
// Packet types
// ============================================================

enum PacketType : uint8_t {
    PKT_TELEMETRY = 0x01,
    PKT_ALERT     = 0x02,
    PKT_ACK       = 0x03,
    PKT_ROUTE     = 0x04,
    PKT_HEARTBEAT = 0x05,
    PKT_CONFIG    = 0x06,
    PKT_DISCOVERY = 0x07
};

// ============================================================
// Hazard types
// ============================================================

enum HazardType : uint8_t {
    HAZARD_NORMAL    = 0,
    HAZARD_FIRE      = 1,
    HAZARD_FLOOD     = 2,
    HAZARD_SEISMIC   = 3,
    HAZARD_POLLUTION = 4,
    HAZARD_MULTI     = 5
};

// ============================================================
// Packet priority
// ============================================================

enum Priority : uint8_t {
    PRIORITY_NORMAL   = 0,
    PRIORITY_HIGH     = 1,
    PRIORITY_CRITICAL = 2
};

// ============================================================
// Packet flags
// ============================================================

enum PacketFlags : uint8_t {

    FLAG_NONE =
        0x00,

    // Payload is encrypted and authenticated.
    FLAG_ENCRYPTED =
        0x01,

    // Sender expects an ACK.
    FLAG_ACK_REQUIRED =
        0x02,

    // Packet has been forwarded by another mesh node.
    FLAG_FORWARDED =
        0x04,

    // Critical event packet.
    FLAG_CRITICAL =
        0x08,

    // Locally detected hazard.
    FLAG_LOCAL_ALERT =
        0x10,

    // Packet was queued because a route/link was unavailable.
    FLAG_STORE_FORWARD =
        0x20
};

// ============================================================
// Sensor status bits
// ============================================================

enum SensorStatusBits : uint32_t {

    SENSOR_STATUS_NONE =
        0x00000000UL,

    SENSOR_STATUS_BME280 =
        1UL << 0,

    SENSOR_STATUS_ADS1115_1 =
        1UL << 1,

    SENSOR_STATUS_ADS1115_2 =
        1UL << 2,

    SENSOR_STATUS_GPS =
        1UL << 3,

    SENSOR_STATUS_MPU6050 =
        1UL << 4,

    SENSOR_STATUS_WEATHER =
        1UL << 5,

    SENSOR_STATUS_DS18B20 =
        1UL << 6,

    SENSOR_STATUS_FLAME =
        1UL << 7,

    SENSOR_STATUS_BATTERY =
        1UL << 8,

    SENSOR_STATUS_LORA =
        1UL << 9,

    SENSOR_STATUS_LOW_BATTERY =
        1UL << 10,

    SENSOR_STATUS_CRITICAL_BATTERY =
        1UL << 11,

    SENSOR_STATUS_SENSOR_ERROR =
        1UL << 12
};

// ============================================================
// ACK status
// ============================================================

enum AckStatus : uint8_t {

    ACK_OK =
        0x00,

    ACK_REJECTED =
        0x01,

    ACK_INVALID =
        0x02,

    ACK_NO_ROUTE =
        0x03,

    ACK_DUPLICATE =
        0x04
};

// ============================================================
// Route message type
// ============================================================

enum RouteMessageType : uint8_t {

    ROUTE_UPDATE =
        0x01,

    ROUTE_REQUEST =
        0x02,

    ROUTE_RESPONSE =
        0x03
};

// ============================================================
// Packet header
//
// IMPORTANT:
//
// TTL and hopCount are intentionally mutable by the mesh router.
// CryptoManager therefore must NOT include these two fields in
// its authenticated associated-data structure.
//
// All other protected header information should be authenticated.
// ============================================================

struct __attribute__((packed)) PacketHeader {

    uint16_t magic;

    uint8_t version;

    uint8_t packetType;

    uint8_t flags;

    uint16_t sourceNode;

    uint16_t destinationNode;

    uint32_t sequence;

    uint8_t ttl;

    uint8_t hopCount;

    uint32_t timestamp;

    uint8_t nonce[12];
};

// ============================================================
// Sensor telemetry payload
//
// Compact binary representation.
// This is transmitted over LoRa rather than JSON.
// ============================================================

struct __attribute__((packed)) SensorPayload {

    int16_t temperature_x100;

    uint16_t humidity_x100;

    uint32_t pressure_pa;

    int32_t latitude_x1e6;

    int32_t longitude_x1e6;

    int16_t altitude_m;

    uint16_t mq2;

    uint16_t mq135;

    uint16_t waterLevel;

    uint16_t soilMoisture;

    int16_t soilTemperature_x100;

    uint16_t vibration_x100;

    uint16_t rainfall_x10;

    uint16_t windSpeed_x10;

    uint16_t turbidity;

    uint16_t ph_x100;

    uint16_t tds;

    uint16_t battery_mV;

    uint8_t riskScore;

    uint8_t hazardType;

    uint32_t sensorStatus;
};

// ============================================================
// Alert payload
// ============================================================

struct __attribute__((packed)) AlertPayload {

    uint8_t hazardType;

    uint8_t priority;

    uint8_t riskScore;

    uint8_t fireRisk;

    uint8_t floodRisk;

    uint8_t seismicRisk;

    uint8_t pollutionRisk;

    uint8_t confidence;

    uint8_t critical;

    uint8_t reserved;

    uint32_t sensorStatus;

    uint32_t detectedAt;
};

// ============================================================
// ACK payload
// ============================================================

struct __attribute__((packed)) AckPayload {

    uint16_t sourceNode;

    uint32_t acknowledgedSequence;

    uint8_t status;

    uint8_t reserved;

    uint32_t timestamp;
};

// ============================================================
// Route payload
// ============================================================

struct __attribute__((packed)) RoutePayload {

    uint8_t routeMessageType;

    uint16_t destinationNode;

    uint16_t nextHop;

    uint8_t hopCount;

    int16_t rssi;

    int16_t snr_x10;

    uint32_t timestamp;
};

// ============================================================
// Heartbeat payload
// ============================================================

struct __attribute__((packed)) HeartbeatPayload {

    uint16_t nodeId;

    uint8_t nodeRole;

    uint8_t batteryState;

    uint16_t battery_mV;

    uint8_t riskScore;

    uint8_t hazardType;

    uint32_t sensorStatus;

    uint32_t uptimeSeconds;
};

// ============================================================
// Discovery payload
// ============================================================

struct __attribute__((packed)) DiscoveryPayload {

    uint16_t nodeId;

    uint8_t nodeRole;

    uint8_t capabilities;

    uint8_t maxTxPower;

    uint8_t reserved;

    uint32_t timestamp;
};

// ============================================================
// Configuration payload
// ============================================================

struct __attribute__((packed)) ConfigPayload {

    uint16_t targetNode;

    uint16_t configurationVersion;

    uint32_t telemetryIntervalMs;

    uint32_t heartbeatIntervalMs;

    uint32_t timestamp;
};

// ============================================================
// Packet container
//
// The payload area stores either:
//
// - encrypted payload
// - plaintext control payload where permitted by protocol
//
// Security policy is enforced by CryptoManager.
// ============================================================

struct __attribute__((packed)) PacketContainer {

    PacketHeader header;

    uint8_t payload[160];

    uint16_t payloadLength;

    uint8_t authenticationTag[16];
};

// ============================================================
// Packet identity
//
// A packet is uniquely identified by:
//     sourceNode + sequence
// ============================================================

struct __attribute__((packed)) PacketId {

    uint16_t sourceNode;

    uint32_t sequence;
};

// ============================================================
// Helper functions
// ============================================================

inline PacketId makePacketId(
    const PacketHeader &header
) {

    PacketId id{};

    id.sourceNode = header.sourceNode;
    id.sequence = header.sequence;

    return id;
}

inline bool samePacket(
    const PacketId &a,
    const PacketId &b
) {

    return
        a.sourceNode == b.sourceNode &&
        a.sequence == b.sequence;
}

inline bool isBroadcast(
    uint16_t nodeId
) {

    return nodeId == BROADCAST_ID;
}

inline bool isGateway(
    uint16_t nodeId
) {

    return nodeId == GATEWAY_ID;
}

inline bool isValidPacketType(
    uint8_t type
) {

    return
        type >= PKT_TELEMETRY &&
        type <= PKT_DISCOVERY;
}

inline bool isValidHazardType(
    uint8_t hazard
) {

    return
        hazard <= HAZARD_MULTI;
}

inline bool isValidPriority(
    uint8_t priority
) {

    return
        priority <= PRIORITY_CRITICAL;
}

inline bool requiresAck(
    const PacketHeader &header
) {

    return
        (header.flags & FLAG_ACK_REQUIRED) != 0;
}

inline bool isEncrypted(
    const PacketHeader &header
) {

    return
        (header.flags & FLAG_ENCRYPTED) != 0;
}

inline bool isCritical(
    const PacketHeader &header
) {

    return
        (header.flags & FLAG_CRITICAL) != 0;
}

inline bool isForwarded(
    const PacketHeader &header
) {

    return
        (header.flags & FLAG_FORWARDED) != 0;
}

} // namespace SankatNet

#endif