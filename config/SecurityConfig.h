#ifndef SANKATNET_SECURITY_CONFIG_H
#define SANKATNET_SECURITY_CONFIG_H

#include <Arduino.h>


namespace SankatNet {
namespace SecurityConfig {


/*
 * ============================================================
 * NETWORK SECURITY
 * ============================================================
 *
 * Sankat-Net uses AES-256-GCM through CryptoManager.
 *
 * AES-256:
 *   256-bit encryption key
 *
 * GCM:
 *   Confidentiality + authentication
 *
 * The key is NEVER transmitted inside a LoRa packet.
 *
 * IMPORTANT:
 * Replace the demonstration key below before deploying
 * physical nodes.
 */


/*
 * 32-byte = 256-bit network key.
 *
 * This is ONLY a placeholder for development/testing.
 *
 * Do NOT commit a real production key to a public Git
 * repository.
 */

static constexpr uint8_t NETWORK_KEY[32] = {

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
 * Key identifier.
 *
 * Useful when rotating network keys.
 *
 * It is NOT the secret key itself.
 */

static constexpr uint8_t KEY_ID =
    0x01;


/*
 * Security protocol version.
 */

static constexpr uint8_t SECURITY_VERSION =
    0x01;


/*
 * AES-GCM parameters.
 */

static constexpr uint8_t AES_KEY_SIZE =
    32;

static constexpr uint8_t GCM_NONCE_SIZE =
    12;

static constexpr uint8_t GCM_TAG_SIZE =
    16;


/*
 * Security policy.
 */

static constexpr bool REQUIRE_ENCRYPTION =
    true;

static constexpr bool REQUIRE_AUTHENTICATION =
    true;


/*
 * Reject packets that are not authenticated.
 */

static constexpr bool
REJECT_UNAUTHENTICATED_PACKETS =
    true;


/*
 * Do not permit plaintext telemetry.
 */

static constexpr bool
ALLOW_PLAINTEXT_TELEMETRY =
    false;


/*
 * Key rotation support.
 *
 * Reserved for future network-wide key rotation.
 */

static constexpr bool
ENABLE_KEY_ROTATION =
    false;


/*
 * Maximum number of keys that may be supported by the
 * security layer in a future implementation.
 */

static constexpr uint8_t
MAX_KEY_SLOTS =
    2;


/*
 * ============================================================
 * DEVELOPMENT MODE
 * ============================================================
 *
 * Development mode can be used while testing individual
 * modules, but should remain disabled for deployment.
 */

static constexpr bool
DEVELOPMENT_MODE =
    true;


/*
 * When DEVELOPMENT_MODE is false, security requirements
 * become mandatory.
 */

static constexpr bool
SECURITY_REQUIRED =
    REQUIRE_ENCRYPTION &&
    REQUIRE_AUTHENTICATION;


/*
 * ============================================================
 * VALIDATION
 * ============================================================
 */

static_assert(
    AES_KEY_SIZE == 32,
    "AES-256 requires a 32-byte key"
);

static_assert(
    GCM_NONCE_SIZE == 12,
    "AES-GCM nonce must use 12 bytes"
);

static_assert(
    GCM_TAG_SIZE == 16,
    "AES-GCM authentication tag must use 16 bytes"
);

static_assert(
    sizeof(NETWORK_KEY) == AES_KEY_SIZE,
    "NETWORK_KEY must be exactly 32 bytes"
);


/*
 * Security configuration must not allow plaintext telemetry
 * when encryption is mandatory.
 */

static_assert(
    !(REQUIRE_ENCRYPTION &&
      ALLOW_PLAINTEXT_TELEMETRY),
    "Plaintext telemetry cannot be enabled with mandatory encryption"
);


} // namespace SecurityConfig
} // namespace SankatNet


#endif