#ifndef SANKATNET_CRYPTO_MANAGER_H
#define SANKATNET_CRYPTO_MANAGER_H

#include <Arduino.h>
#include <mbedtls/gcm.h>
#include <esp_system.h>

#include "Packet.h"
#include "../config/SecurityConfig.h"


namespace SankatNet {


class CryptoManager {

public:

    // ========================================================
    // CRYPTO CONSTANTS
    // ========================================================

    static constexpr size_t KEY_SIZE =
        SecurityConfig::AES_KEY_SIZE;

    static constexpr size_t NONCE_SIZE =
        SecurityConfig::GCM_NONCE_SIZE;

    static constexpr size_t TAG_SIZE =
        SecurityConfig::GCM_TAG_SIZE;


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin()
    {
        if (
            initialized
        ) {
            return true;
        }


        /*
         * Copy the network key into private RAM.
         */

        memcpy(
            networkKey,
            SecurityConfig::NETWORK_KEY,
            KEY_SIZE
        );


        mbedtls_gcm_init(
            &gcm
        );


        int result =
            mbedtls_gcm_setkey(
                &gcm,
                MBEDTLS_CIPHER_ID_AES,
                networkKey,
                KEY_SIZE * 8
            );


        if (
            result != 0
        ) {

            mbedtls_gcm_free(
                &gcm
            );

            initialized =
                false;

            return false;
        }


        initialized =
            true;


        return true;
    }


    // ========================================================
    // DESTRUCTOR
    // ========================================================

    ~CryptoManager()
    {
        mbedtls_gcm_free(
            &gcm
        );


        /*
         * Clear sensitive key material from RAM.
         */

        memset(
            networkKey,
            0,
            sizeof(networkKey)
        );
    }


    // ========================================================
    // ENCRYPT
    // ========================================================

    bool encrypt(
        const PacketHeader& header,
        const uint8_t* plaintext,
        size_t plaintextLength,
        uint8_t* ciphertext,
        uint8_t* authenticationTag
    )
    {
        if (
            !initialized ||
            plaintext == nullptr ||
            ciphertext == nullptr ||
            authenticationTag == nullptr
        ) {
            return false;
        }


        if (
            plaintextLength >
            MAX_ENCRYPTED_PAYLOAD
        ) {
            return false;
        }


        /*
         * Build immutable authenticated data.
         *
         * TTL and hopCount are intentionally excluded because
         * the mesh router may modify them while forwarding.
         */

        AuthenticatedHeader aad;

        buildAuthenticatedHeader(
            header,
            aad
        );


        int result =
            mbedtls_gcm_crypt_and_tag(
                &gcm,
                MBEDTLS_GCM_ENCRYPT,
                plaintextLength,
                header.nonce,
                NONCE_SIZE,
                reinterpret_cast<const uint8_t*>(&aad),
                sizeof(aad),
                plaintext,
                ciphertext,
                TAG_SIZE,
                authenticationTag
            );


        if (
            result != 0
        ) {

            encryptionFailures++;

            return false;
        }


        encryptedPackets++;

        return true;
    }


    // ========================================================
    // DECRYPT
    // ========================================================

    bool decrypt(
        const PacketHeader& header,
        const uint8_t* ciphertext,
        size_t ciphertextLength,
        const uint8_t* authenticationTag,
        uint8_t* plaintext
    )
    {
        if (
            !initialized ||
            ciphertext == nullptr ||
            authenticationTag == nullptr ||
            plaintext == nullptr
        ) {
            return false;
        }


        if (
            ciphertextLength >
            MAX_ENCRYPTED_PAYLOAD
        ) {
            return false;
        }


        AuthenticatedHeader aad;

        buildAuthenticatedHeader(
            header,
            aad
        );


        int result =
            mbedtls_gcm_auth_decrypt(
                &gcm,
                ciphertextLength,
                header.nonce,
                NONCE_SIZE,
                reinterpret_cast<const uint8_t*>(&aad),
                sizeof(aad),
                authenticationTag,
                TAG_SIZE,
                ciphertext,
                plaintext
            );


        if (
            result != 0
        ) {

            authenticationFailures++;

            /*
             * Never leave unauthenticated plaintext available.
             */

            memset(
                plaintext,
                0,
                ciphertextLength
            );

            return false;
        }


        decryptedPackets++;

        return true;
    }


    // ========================================================
    // ENCRYPT PACKET
    // ========================================================

    bool encryptPacket(
        PacketContainer& packet
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        if (
            packet.payloadLength >
            MAX_ENCRYPTED_PAYLOAD
        ) {
            return false;
        }


        /*
         * Set encryption flag BEFORE the header is authenticated.
         */

        packet.header.flags |=
            FLAG_ENCRYPTED;


        /*
         * Generate a fresh nonce.
         */

        if (
            !generateNonce(
                packet.header.sourceNode,
                packet.header.sequence,
                packet.header.nonce
            )
        ) {

            packet.header.flags &=
                ~FLAG_ENCRYPTED;

            return false;
        }


        /*
         * Encrypt payload in-place.
         */

        uint8_t encryptedPayload[
            MAX_ENCRYPTED_PAYLOAD
        ];


        if (
            !encrypt(
                packet.header,
                packet.payload,
                packet.payloadLength,
                encryptedPayload,
                packet.authenticationTag
            )
        ) {

            packet.header.flags &=
                ~FLAG_ENCRYPTED;

            return false;
        }


        memcpy(
            packet.payload,
            encryptedPayload,
            packet.payloadLength
        );


        return true;
    }


    // ========================================================
    // DECRYPT PACKET
    // ========================================================

    bool decryptPacket(
        PacketContainer& packet
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        if (
            !(packet.header.flags &
              FLAG_ENCRYPTED)
        ) {

            if (
                SecurityConfig::
                REJECT_UNAUTHENTICATED_PACKETS
            ) {

                return false;
            }


            return true;
        }


        if (
            packet.payloadLength >
            MAX_ENCRYPTED_PAYLOAD
        ) {
            return false;
        }


        uint8_t decryptedPayload[
            MAX_ENCRYPTED_PAYLOAD
        ];


        if (
            !decrypt(
                packet.header,
                packet.payload,
                packet.payloadLength,
                packet.authenticationTag,
                decryptedPayload
            )
        ) {

            return false;
        }


        memcpy(
            packet.payload,
            decryptedPayload,
            packet.payloadLength
        );


        packet.header.flags &=
            ~FLAG_ENCRYPTED;


        return true;
    }

    bool authenticatePacket(
        const PacketContainer& packet
    )
    {
        if (!initialized ||
            !(packet.header.flags & FLAG_ENCRYPTED) ||
            packet.payloadLength > MAX_ENCRYPTED_PAYLOAD) {
            return false;
        }

        uint8_t plaintext[MAX_ENCRYPTED_PAYLOAD] = {};

        return decrypt(
            packet.header,
            packet.payload,
            packet.payloadLength,
            packet.authenticationTag,
            plaintext
        );
    }


    // ========================================================
    // NONCE GENERATION
    // ========================================================

    bool generateNonce(
        uint16_t sourceNode,
        uint32_t sequence,
        uint8_t nonce[NONCE_SIZE]
    )
    {
        if (
            nonce == nullptr
        ) {
            return false;
        }


        /*
         * First part binds the nonce to the packet identity.
         */

        nonce[0] =
            static_cast<uint8_t>(
                sourceNode >> 8
            );

        nonce[1] =
            static_cast<uint8_t>(
                sourceNode
            );


        nonce[2] =
            static_cast<uint8_t>(
                sequence >> 24
            );

        nonce[3] =
            static_cast<uint8_t>(
                sequence >> 16
            );

        nonce[4] =
            static_cast<uint8_t>(
                sequence >> 8
            );

        nonce[5] =
            static_cast<uint8_t>(
                sequence
            );


        /*
         * Remaining bytes come from the ESP32 hardware RNG.
         */

        uint32_t randomValue;


        for (
            uint8_t i = 6;
            i < NONCE_SIZE;
            i += 4
        ) {

            randomValue =
                esp_random();


            for (
                uint8_t j = 0;
                j < 4 &&
                (i + j) < NONCE_SIZE;
                j++
            ) {

                nonce[i + j] =
                    static_cast<uint8_t>(
                        randomValue >>
                        (j * 8)
                    );
            }
        }


        return true;
    }


    // ========================================================
    // STATUS
    // ========================================================

    bool isInitialized() const
    {
        return initialized;
    }


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t getEncryptedPackets() const
    {
        return encryptedPackets;
    }


    uint32_t getDecryptedPackets() const
    {
        return decryptedPackets;
    }


    uint32_t getEncryptionFailures() const
    {
        return encryptionFailures;
    }


    uint32_t getAuthenticationFailures() const
    {
        return authenticationFailures;
    }


private:

    // ========================================================
    // AUTHENTICATED HEADER
    // ========================================================

    /*
     * IMPORTANT:
     *
     * TTL and hopCount are intentionally absent.
     *
     * These fields are mutable routing metadata and are changed
     * by MeshRouter when a packet is forwarded.
     *
     * The following fields remain authenticated:
     *
     *   magic
     *   version
     *   packetType
     *   flags
     *   sourceNode
     *   destinationNode
     *   sequence
     *   timestamp
     *   nonce
     *
     * Therefore a relay can modify TTL/hopCount without
     * invalidating the end-to-end authentication tag.
     */

    struct __attribute__((packed)) AuthenticatedHeader {

        uint16_t magic;

        uint8_t version;

        uint8_t packetType;

        uint8_t flags;

        uint16_t sourceNode;

        uint16_t destinationNode;

        uint32_t sequence;

        uint32_t timestamp;

        uint8_t nonce[NONCE_SIZE];
    };


    // ========================================================
    // BUILD AAD
    // ========================================================

    void buildAuthenticatedHeader(
        const PacketHeader& header,
        AuthenticatedHeader& aad
    ) const
    {
        aad.magic =
            header.magic;

        aad.version =
            header.version;

        aad.packetType =
            header.packetType;

        aad.flags =
            header.flags;

        aad.sourceNode =
            header.sourceNode;

        aad.destinationNode =
            header.destinationNode;

        aad.sequence =
            header.sequence;

        aad.timestamp =
            header.timestamp;


        memcpy(
            aad.nonce,
            header.nonce,
            NONCE_SIZE
        );
    }


    // ========================================================
    // MEMBERS
    // ========================================================

    mbedtls_gcm_context gcm;

    uint8_t networkKey[
        KEY_SIZE
    ] = {};


    bool initialized =
        false;


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t encryptedPackets =
        0;

    uint32_t decryptedPackets =
        0;

    uint32_t encryptionFailures =
        0;

    uint32_t authenticationFailures =
        0;
};


} // namespace SankatNet


#endif