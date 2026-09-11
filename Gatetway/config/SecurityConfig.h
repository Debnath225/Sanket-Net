#ifndef SANKATNET_GATEWAY_SECURITY_CONFIG_H
#define SANKATNET_GATEWAY_SECURITY_CONFIG_H

#include <Arduino.h>

/*
 * ============================================================
 * Sankat-Net Gateway Security Configuration
 * ============================================================
 *
 * IMPORTANT:
 * Replace the development key before deployment.
 *
 * The LoRa network uses the same 256-bit network key as the
 * field nodes so that authenticated encrypted packets can be
 * verified and decrypted at the gateway.
 *
 * Do NOT commit the real production key to a public GitHub
 * repository.
 * ============================================================
 */


/*
 * ============================================================
 * LORA NETWORK SECURITY
 * ============================================================
 */

#define SANKATNET_LORA_ENCRYPTION_ENABLED   true

#define SANKATNET_LORA_REQUIRE_ENCRYPTION   true


/*
 * 32-byte AES-256 development key.
 *
 * Replace every byte with a securely generated random
 * production key before deployment.
 */

static const uint8_t
SANKATNET_LORA_NETWORK_KEY[32] =
{
    0x53, 0x61, 0x6E, 0x6B,
    0x61, 0x74, 0x4E, 0x65,
    0x74, 0x2D, 0x44, 0x65,
    0x76, 0x2D, 0x4B, 0x65,
    0x79, 0x2D, 0x32, 0x30,
    0x32, 0x36, 0x2D, 0x45,
    0x53, 0x50, 0x33, 0x32,
    0x2D, 0x58, 0x31
};


/*
 * ============================================================
 * AES CONFIGURATION
 * ============================================================
 */

#define SANKATNET_AES_KEY_SIZE        32

#define SANKATNET_AES_NONCE_SIZE      12

#define SANKATNET_AES_TAG_SIZE        16


/*
 * ============================================================
 * REPLAY PROTECTION
 * ============================================================
 */

#define SANKATNET_REPLAY_PROTECTION_ENABLED   true

/*
 * Maximum acceptable timestamp drift.
 *
 * The exact replay implementation remains in
 * ReplayProtection.h.
 */

#define SANKATNET_MAX_TIMESTAMP_DRIFT_MS       300000UL


/*
 * ============================================================
 * PACKET SECURITY
 * ============================================================
 */

#define SANKATNET_REQUIRE_PACKET_AUTHENTICATION   true

#define SANKATNET_REQUIRE_PACKET_CRC              true

namespace SecurityConfig {
constexpr bool SECURITY_REQUIRED =
    SANKATNET_REQUIRE_PACKET_AUTHENTICATION;
}


/*
 * ============================================================
 * SECURITY FAILURE HANDLING
 * ============================================================
 */

#define SANKATNET_MAX_SECURITY_FAILURES          10

#define SANKATNET_SECURITY_FAILURE_WINDOW_MS     60000UL


/*
 * ============================================================
 * MQTT / INTERNET SECURITY
 * ============================================================
 *
 * MQTT must use TLS in production.
 */

#define SANKATNET_MQTT_TLS_REQUIRED               true

#define SANKATNET_MQTT_TLS_PORT                   8883


/*
 * MQTT credentials are intentionally NOT stored here.
 *
 * Define them through the build environment or a private
 * configuration header:
 *
 * SANKATNET_MQTT_BROKER
 * SANKATNET_MQTT_USERNAME
 * SANKATNET_MQTT_PASSWORD
 * SANKATNET_MQTT_ROOT_CA
 */


/*
 * ============================================================
 * SECURE BOOT / FLASH ENCRYPTION
 * ============================================================
 *
 * These are deployment-level ESP32 security features.
 *
 * They are not enabled from this header because the exact
 * configuration depends on the ESP32 board/chip and
 * production provisioning process.
 */

#define SANKATNET_SECURE_BOOT_REQUIRED             false

#define SANKATNET_FLASH_ENCRYPTION_REQUIRED        false


/*
 * ============================================================
 * DEBUG SECURITY LOGGING
 * ============================================================
 *
 * Never print:
 * - AES keys
 * - MQTT passwords
 * - TLS private credentials
 * - decrypted packet contents unnecessarily
 */

#define SANKATNET_SECURITY_DEBUG_LOGGING            true


/*
 * ============================================================
 * STATIC VALIDATION
 * ============================================================
 */

static_assert(
    SANKATNET_AES_KEY_SIZE == 32,
    "AES-256 requires a 32-byte key"
);

static_assert(
    SANKATNET_AES_NONCE_SIZE == 12,
    "AES-GCM nonce must be 12 bytes"
);

static_assert(
    SANKATNET_AES_TAG_SIZE == 16,
    "AES-GCM authentication tag must be 16 bytes"
);

static_assert(
    sizeof(SANKATNET_LORA_NETWORK_KEY) ==
    SANKATNET_AES_KEY_SIZE,
    "Invalid AES-256 network key size"
);


#endif