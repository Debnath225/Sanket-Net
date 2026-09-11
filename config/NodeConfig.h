#ifndef SANKATNET_NODE_CONFIG_H
#define SANKATNET_NODE_CONFIG_H

#include <Arduino.h>

/*
 * ============================================================
 * Sankat-Net : Node Configuration
 * ============================================================
 *
 * This file contains node-specific and network-wide settings.
 *
 * IMPORTANT:
 * - Do NOT store encryption keys here.
 * - Cryptographic secrets belong in SecurityConfig.h / NVS.
 * - Every node is capable of mesh forwarding.
 * - Change NODE_ID and NODE_ROLE for each physical node.
 *
 * ============================================================
 */

namespace SankatNet {

// ============================================================
// 1. NODE IDENTITY
// ============================================================

/*
 * Node IDs:
 *   0x0001 -> Fire / Pollution Node
 *   0x0002 -> Flood / Soil Node
 *   0x0003 -> Seismic / Weather Node
 *   0x0004 -> Water Quality Node
 *   0xFFFF -> Gateway / Broadcast destination
 */

constexpr uint16_t GATEWAY_ID   = 0xFFFF;
constexpr uint16_t BROADCAST_ID = 0xFFFE;

/*
 * Change this value for each ESP32.
 *
 * NODE 1:
 *   0x0001
 *
 * NODE 2:
 *   0x0002
 *
 * NODE 3:
 *   0x0003
 *
 * NODE 4:
 *   0x0004
 */
#ifndef SANKAT_NODE_ID
#define SANKAT_NODE_ID 0x0001
#endif

constexpr uint16_t NODE_ID = SANKAT_NODE_ID;


// ============================================================
// 2. NODE ROLES
// ============================================================

enum class NodeRole : uint8_t {
    FIRE_POLLUTION = 0x01,
    FLOOD_SOIL     = 0x02,
    SEISMIC_WEATHER = 0x03,
    WATER_QUALITY  = 0x04,
    GATEWAY        = 0x05
};

#ifndef SANKAT_NODE_ROLE
#define SANKAT_NODE_ROLE NodeRole::FIRE_POLLUTION
#endif

constexpr NodeRole NODE_ROLE = SANKAT_NODE_ROLE;


// ============================================================
// 3. SENSOR / NODE CAPABILITIES
// ============================================================
//
// A bitmask allows the gateway to know what a node is capable
// of sensing without sending a long text description.
//

enum Capability : uint32_t {

    CAP_BME280          = (1UL << 0),
    CAP_MQ2             = (1UL << 1),
    CAP_MQ135           = (1UL << 2),
    CAP_FLAME           = (1UL << 3),

    CAP_WATER_LEVEL     = (1UL << 4),
    CAP_SOIL_MOISTURE   = (1UL << 5),
    CAP_SOIL_TEMPERATURE = (1UL << 6),

    CAP_MPU6050         = (1UL << 7),

    CAP_RAINFALL        = (1UL << 8),
    CAP_WIND_SPEED      = (1UL << 9),

    CAP_TURBIDITY       = (1UL << 10),
    CAP_PH              = (1UL << 11),
    CAP_TDS             = (1UL << 12),

    CAP_GPS             = (1UL << 13),
    CAP_BATTERY         = (1UL << 14),

    /*
     * Every field node has these network capabilities.
     */
    CAP_LORA            = (1UL << 15),
    CAP_MESH_ROUTER     = (1UL << 16),
    CAP_STORE_FORWARD   = (1UL << 17),
    CAP_LOCAL_RISK      = (1UL << 18)
};


// ============================================================
// 4. ROLE CAPABILITY PROFILES
// ============================================================

constexpr uint32_t FIRE_POLLUTION_CAPS =
      CAP_BME280
    | CAP_MQ2
    | CAP_MQ135
    | CAP_FLAME
    | CAP_GPS
    | CAP_BATTERY
    | CAP_LORA
    | CAP_MESH_ROUTER
    | CAP_STORE_FORWARD
    | CAP_LOCAL_RISK;

constexpr uint32_t FLOOD_SOIL_CAPS =
      CAP_BME280
    | CAP_WATER_LEVEL
    | CAP_SOIL_MOISTURE
    | CAP_SOIL_TEMPERATURE
    | CAP_RAINFALL
    | CAP_GPS
    | CAP_BATTERY
    | CAP_LORA
    | CAP_MESH_ROUTER
    | CAP_STORE_FORWARD
    | CAP_LOCAL_RISK;

constexpr uint32_t SEISMIC_WEATHER_CAPS =
      CAP_BME280
    | CAP_MPU6050
    | CAP_WIND_SPEED
    | CAP_GPS
    | CAP_BATTERY
    | CAP_LORA
    | CAP_MESH_ROUTER
    | CAP_STORE_FORWARD
    | CAP_LOCAL_RISK;

constexpr uint32_t WATER_QUALITY_CAPS =
      CAP_BME280
    | CAP_TURBIDITY
    | CAP_PH
    | CAP_TDS
    | CAP_GPS
    | CAP_BATTERY
    | CAP_LORA
    | CAP_MESH_ROUTER
    | CAP_STORE_FORWARD
    | CAP_LOCAL_RISK;


// ============================================================
// 5. GET CAPABILITIES FOR CURRENT NODE
// ============================================================

constexpr uint32_t getNodeCapabilities()
{
    switch (NODE_ROLE) {

        case NodeRole::FIRE_POLLUTION:
            return FIRE_POLLUTION_CAPS;

        case NodeRole::FLOOD_SOIL:
            return FLOOD_SOIL_CAPS;

        case NodeRole::SEISMIC_WEATHER:
            return SEISMIC_WEATHER_CAPS;

        case NodeRole::WATER_QUALITY:
            return WATER_QUALITY_CAPS;

        case NodeRole::GATEWAY:
            return CAP_LORA
                 | CAP_MESH_ROUTER
                 | CAP_STORE_FORWARD;

        default:
            return 0;
    }
}

constexpr uint32_t NODE_CAPABILITIES = getNodeCapabilities();


// ============================================================
// 6. LORA RADIO CONFIGURATION
// ============================================================

namespace LoRaConfig {

    /*
     * Frequency must match the radio hardware and the
     * legally permitted band/configuration for deployment.
     *
     * This is the default project value.
     */
    constexpr long FREQUENCY = 433E6;

    /*
     * LoRa bandwidth:
     * 125 kHz
     */
    constexpr long BANDWIDTH = 125E3;

    /*
     * Spreading Factor:
     * SF9 gives a useful range/reliability trade-off.
     */
    constexpr uint8_t SPREADING_FACTOR = 9;

    /*
     * Coding Rate = 4/5
     */
    constexpr uint8_t CODING_RATE = 5;

    /*
     * Sync word used by Sankat-Net nodes.
     */
    constexpr uint8_t SYNC_WORD = 0x12;

    /*
     * TX power.
     *
     * Actual permitted power depends on the radio/module,
     * antenna and applicable regulations.
     */
    constexpr int8_t TX_POWER_DBM = 17;

    /*
     * Preamble length.
     */
    constexpr uint16_t PREAMBLE_LENGTH = 8;

    /*
     * Enable CRC at LoRa PHY level.
     */
    constexpr bool ENABLE_CRC = true;

}


// ============================================================
// 7. PACKET / RADIO LIMITS
// ============================================================

/*
 * SX1278 supports up to 255 bytes of payload.
 *
 * We deliberately keep our application packet below the
 * theoretical maximum to leave room for future protocol
 * extensions and safer airtime management.
 */
constexpr uint16_t MAX_LORA_PACKET_SIZE = 220;

/*
 * Maximum encrypted application payload.
 */
constexpr uint16_t MAX_ENCRYPTED_PAYLOAD = 160;


// ============================================================
// 8. MESH ROUTING PARAMETERS
// ============================================================

namespace MeshConfig {

    /*
     * Maximum number of hops a packet may travel.
     *
     * Example:
     * NODE-01 -> NODE-03 -> NODE-02 -> GATEWAY
     *
     * Each forwarding node decrements TTL.
     */
    constexpr uint8_t DEFAULT_TTL = 8;

    /*
     * Maximum number of forwarding attempts before a packet
     * is discarded.
     */
    constexpr uint8_t MAX_FORWARD_HOPS = 8;

    /*
     * Every node participates in routing.
     */
    constexpr bool ROUTING_ENABLED = true;

    /*
     * Store-and-forward is enabled.
     */
    constexpr bool STORE_AND_FORWARD_ENABLED = true;

    /*
     * Maximum number of packets retained in the local queue.
     */
    constexpr uint8_t PACKET_QUEUE_SIZE = 24;

    /*
     * Duplicate packet cache.
     */
    constexpr uint8_t DUPLICATE_CACHE_SIZE = 32;

}

constexpr uint8_t MAX_TTL = MeshConfig::DEFAULT_TTL;
constexpr uint8_t MAX_HOPS = MeshConfig::MAX_FORWARD_HOPS;


// ============================================================
// 9. TELEMETRY TIMING
// ============================================================

namespace Timing {

    /*
     * Normal environmental telemetry interval.
     */
    constexpr uint32_t TELEMETRY_INTERVAL_MS = 30000UL;

    /*
     * Heartbeat interval.
     */
    constexpr uint32_t HEARTBEAT_INTERVAL_MS = 60000UL;

    /*
     * Route/discovery maintenance interval.
     */
    constexpr uint32_t ROUTE_UPDATE_INTERVAL_MS = 120000UL;

    /*
     * ACK waiting period.
     */
    constexpr uint32_t ACK_TIMEOUT_MS = 1500UL;

    /*
     * Maximum retransmission attempts.
     */
    constexpr uint8_t MAX_RETRIES = 3;

    /*
     * Delay between retransmissions.
     */
    constexpr uint32_t RETRY_DELAY_MS = 250UL;

}


// ============================================================
// 10. ALERT TIMING
// ============================================================

namespace AlertConfig {

    /*
     * Critical alerts should not wait for the normal telemetry
     * cycle.
     */
    constexpr bool IMMEDIATE_CRITICAL_ALERT = true;

    /*
     * Minimum time between repeated alerts for the same hazard.
     * Prevents radio flooding when a sensor remains above threshold.
     */
    constexpr uint32_t ALERT_REPEAT_INTERVAL_MS = 10000UL;

    /*
     * Critical alerts get maximum forwarding priority.
     */
    constexpr bool PRIORITIZE_CRITICAL = true;

}


// ============================================================
// 11. SENSOR SAMPLING
// ============================================================

namespace SensorTiming {

    /*
     * Sensor measurements can be sampled more frequently than
     * telemetry packets are transmitted.
     */
    constexpr uint32_t SENSOR_SAMPLE_INTERVAL_MS = 5000UL;

    /*
     * Fast sampling for vibration/motion.
     */
    constexpr uint32_t MOTION_SAMPLE_INTERVAL_MS = 100UL;

    /*
     * GPS update interval.
     */
    constexpr uint32_t GPS_UPDATE_INTERVAL_MS = 1000UL;

}


// ============================================================
// 12. BATTERY MANAGEMENT
// ============================================================

namespace BatteryConfig {

    /*
     * Battery voltage below this level should trigger a
     * low-battery warning.
     */
    constexpr uint16_t LOW_BATTERY_MV = 3500;

    /*
     * Critical battery level.
     */
    constexpr uint16_t CRITICAL_BATTERY_MV = 3300;

    /*
     * Battery measurement interval.
     */
    constexpr uint32_t CHECK_INTERVAL_MS = 10000UL;

}


// ============================================================
// 13. SYSTEM LIMITS
// ============================================================

namespace SystemLimits {

    /*
     * Number of consecutive communication failures before
     * the node considers the route unhealthy.
     */
    constexpr uint8_t MAX_COMM_FAILURES = 5;

    /*
     * Number of consecutive sensor failures before marking
     * a sensor as unhealthy.
     */
    constexpr uint8_t MAX_SENSOR_FAILURES = 3;

    /*
     * Maximum time a queued packet may remain locally stored.
     */
    constexpr uint32_t MAX_QUEUE_AGE_MS =
        30UL * 60UL * 1000UL;   // 30 minutes

}


// ============================================================
// 14. CONFIGURATION VALIDATION
// ============================================================

static_assert(
    NODE_ID != 0x0000,
    "NODE_ID 0x0000 is reserved"
);

static_assert(
    NODE_ID != GATEWAY_ID,
    "Field node cannot use GATEWAY_ID"
);

static_assert(
    NODE_ID != BROADCAST_ID,
    "Field node cannot use BROADCAST_ID"
);

static_assert(
    MAX_ENCRYPTED_PAYLOAD < MAX_LORA_PACKET_SIZE,
    "Encrypted payload exceeds packet limit"
);

static_assert(
    MeshConfig::DEFAULT_TTL > 0,
    "Mesh TTL must be greater than zero"
);

} // namespace SankatNet

#endif