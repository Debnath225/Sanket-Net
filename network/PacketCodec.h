#ifndef SANKATNET_PACKET_CODEC_H
#define SANKATNET_PACKET_CODEC_H

#include <Arduino.h>

#include "Packet.h"
#include "../config/NodeConfig.h"

namespace SankatNet {

class PacketCodec {
public:

    // ============================================================
    // Packet size helpers
    // ============================================================

    static constexpr size_t HEADER_SIZE =
        sizeof(PacketHeader);

    static constexpr size_t TAG_SIZE = 16;

    static constexpr size_t MAX_PAYLOAD_SIZE = 160;

    static constexpr size_t MAX_PACKET_SIZE =
        HEADER_SIZE +
        MAX_PAYLOAD_SIZE +
        sizeof(uint16_t) +
        TAG_SIZE;

    // ------------------------------------------------------------
    // Minimum valid packet size
    // ------------------------------------------------------------
    static constexpr size_t MIN_PACKET_SIZE =
        HEADER_SIZE +
        sizeof(uint16_t) +
        TAG_SIZE;

    // ============================================================
    // Encode PacketContainer → byte buffer
    // ============================================================

    static bool encode(
        const PacketContainer &packet,
        uint8_t *buffer,
        size_t bufferSize,
        size_t &encodedLength
    ) {

        encodedLength = 0;

        if (buffer == nullptr) {
            return false;
        }

        if (packet.payloadLength > MAX_PAYLOAD_SIZE) {
            return false;
        }

        const size_t required =
            HEADER_SIZE +
            packet.payloadLength +
            sizeof(uint16_t) +
            TAG_SIZE;

        if (required > bufferSize ||
            required > MAX_PACKET_SIZE ||
            required > MAX_LORA_PACKET_SIZE) {
            return false;
        }

        // --------------------------------------------------------
        // Validate packet before serialization
        // --------------------------------------------------------
        if (!validateHeader(packet.header)) {
            return false;
        }

        size_t offset = 0;

        // --------------------------------------------------------
        // Header
        // --------------------------------------------------------
        memcpy(
            buffer + offset,
            &packet.header,
            HEADER_SIZE
        );

        offset += HEADER_SIZE;

        // --------------------------------------------------------
        // Payload
        // --------------------------------------------------------
        if (packet.payloadLength > 0) {

            memcpy(
                buffer + offset,
                packet.payload,
                packet.payloadLength
            );

            offset += packet.payloadLength;
        }

        // --------------------------------------------------------
        // Payload length
        // --------------------------------------------------------
        memcpy(
            buffer + offset,
            &packet.payloadLength,
            sizeof(packet.payloadLength)
        );

        offset += sizeof(packet.payloadLength);

        // --------------------------------------------------------
        // Authentication tag
        // --------------------------------------------------------
        memcpy(
            buffer + offset,
            packet.authenticationTag,
            TAG_SIZE
        );

        offset += TAG_SIZE;

        encodedLength = offset;

        return true;
    }

    // ============================================================
    // Decode byte buffer → PacketContainer
    // ============================================================

    static bool decode(
        const uint8_t *buffer,
        size_t bufferLength,
        PacketContainer &packet
    ) {

        if (buffer == nullptr) {
            return false;
        }

        if (bufferLength < MIN_PACKET_SIZE ||
            bufferLength > MAX_PACKET_SIZE ||
            bufferLength > MAX_LORA_PACKET_SIZE) {
            return false;
        }

        size_t offset = 0;

        // --------------------------------------------------------
        // Header
        // --------------------------------------------------------
        memcpy(
            &packet.header,
            buffer + offset,
            HEADER_SIZE
        );

        offset += HEADER_SIZE;

        // --------------------------------------------------------
        // Basic header validation
        // --------------------------------------------------------
        if (!validateHeader(packet.header)) {
            return false;
        }

        // --------------------------------------------------------
        // Payload length
        //
        // The length field is stored immediately before the
        // authentication tag.
        // --------------------------------------------------------
        const size_t payloadLengthOffset =
            bufferLength -
            TAG_SIZE -
            sizeof(uint16_t);

        if (payloadLengthOffset < HEADER_SIZE) {
            return false;
        }

        uint16_t payloadLength = 0;

        memcpy(
            &payloadLength,
            buffer + payloadLengthOffset,
            sizeof(payloadLength)
        );

        if (payloadLength > MAX_PAYLOAD_SIZE) {
            return false;
        }

        const size_t expectedLength =
            HEADER_SIZE +
            payloadLength +
            sizeof(uint16_t) +
            TAG_SIZE;

        if (expectedLength != bufferLength) {
            return false;
        }

        // --------------------------------------------------------
        // Payload
        // --------------------------------------------------------
        packet.payloadLength = payloadLength;

        if (payloadLength > 0) {

            memcpy(
                packet.payload,
                buffer + HEADER_SIZE,
                payloadLength
            );
        }

        // --------------------------------------------------------
        // Authentication tag
        // --------------------------------------------------------
        memcpy(
            packet.authenticationTag,
            buffer + payloadLengthOffset +
                sizeof(uint16_t),
            TAG_SIZE
        );

        return true;
    }

    // ============================================================
    // Header validation
    // ============================================================

    static bool validateHeader(
        const PacketHeader &header
    ) {

        // --------------------------------------------------------
        // Magic
        // --------------------------------------------------------
        if (header.magic != PACKET_MAGIC) {
            return false;
        }

        // --------------------------------------------------------
        // Protocol version
        // --------------------------------------------------------
        if (header.version != PACKET_VERSION) {
            return false;
        }

        // --------------------------------------------------------
        // Packet type
        // --------------------------------------------------------
        if (!isValidPacketType(header.packetType)) {
            return false;
        }

        // --------------------------------------------------------
        // TTL
        // --------------------------------------------------------
        if (header.ttl == 0 ||
            header.ttl > MAX_TTL) {
            return false;
        }

        // --------------------------------------------------------
        // Hop count
        // --------------------------------------------------------
        if (header.hopCount > MAX_HOPS) {
            return false;
        }

        return true;
    }

    // ============================================================
    // Payload encoding helpers
    // ============================================================

    static bool encodeSensorPayload(
        const SensorPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(SensorPayload)
        );
    }

    static bool encodeAlertPayload(
        const AlertPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(AlertPayload)
        );
    }

    static bool encodeAckPayload(
        const AckPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(AckPayload)
        );
    }

    static bool encodeRoutePayload(
        const RoutePayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(RoutePayload)
        );
    }

    static bool encodeHeartbeatPayload(
        const HeartbeatPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(HeartbeatPayload)
        );
    }

    static bool encodeDiscoveryPayload(
        const DiscoveryPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(DiscoveryPayload)
        );
    }

    static bool encodeConfigPayload(
        const ConfigPayload &payload,
        PacketContainer &packet
    ) {

        return setPayload(
            packet,
            &payload,
            sizeof(ConfigPayload)
        );
    }

    // ============================================================
    // Payload decoding helpers
    // ============================================================

    static bool decodeSensorPayload(
        const PacketContainer &packet,
        SensorPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(SensorPayload)
        );
    }

    static bool decodeAlertPayload(
        const PacketContainer &packet,
        AlertPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(AlertPayload)
        );
    }

    static bool decodeAckPayload(
        const PacketContainer &packet,
        AckPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(AckPayload)
        );
    }

    static bool decodeRoutePayload(
        const PacketContainer &packet,
        RoutePayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(RoutePayload)
        );
    }

    static bool decodeHeartbeatPayload(
        const PacketContainer &packet,
        HeartbeatPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(HeartbeatPayload)
        );
    }

    static bool decodeDiscoveryPayload(
        const PacketContainer &packet,
        DiscoveryPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(DiscoveryPayload)
        );
    }

    static bool decodeConfigPayload(
        const PacketContainer &packet,
        ConfigPayload &payload
    ) {

        return getPayload(
            packet,
            &payload,
            sizeof(ConfigPayload)
        );
    }

    // ============================================================
    // Determine expected payload size
    // ============================================================

    static size_t expectedPayloadSize(
        uint8_t packetType
    ) {

        switch (packetType) {

            case PKT_TELEMETRY:
                return sizeof(SensorPayload);

            case PKT_ALERT:
                return sizeof(AlertPayload);

            case PKT_ACK:
                return sizeof(AckPayload);

            case PKT_ROUTE:
                return sizeof(RoutePayload);

            case PKT_HEARTBEAT:
                return sizeof(HeartbeatPayload);

            case PKT_CONFIG:
                return sizeof(ConfigPayload);

            case PKT_DISCOVERY:
                return sizeof(DiscoveryPayload);

            default:
                return 0;
        }
    }

    // ============================================================
    // Validate payload against packet type
    //
    // This should normally be called before interpreting a
    // decrypted payload.
    // ============================================================

    static bool validatePayloadType(
        const PacketContainer &packet
    ) {

        const size_t expected =
            expectedPayloadSize(packet.header.packetType);

        if (expected == 0) {
            return false;
        }

        return packet.payloadLength == expected;
    }

    // ============================================================
    // Clear packet
    // ============================================================

    static void clear(
        PacketContainer &packet
    ) {

        memset(
            &packet,
            0,
            sizeof(PacketContainer)
        );
    }

    // ============================================================
    // Packet total size
    // ============================================================

    static size_t encodedSize(
        const PacketContainer &packet
    ) {

        if (packet.payloadLength > MAX_PAYLOAD_SIZE) {
            return 0;
        }

        return
            HEADER_SIZE +
            packet.payloadLength +
            sizeof(uint16_t) +
            TAG_SIZE;
    }

private:

    // ============================================================
    // Generic payload setter
    // ============================================================

    static bool setPayload(
        PacketContainer &packet,
        const void *source,
        size_t length
    ) {

        if (source == nullptr) {
            return false;
        }

        if (length > MAX_PAYLOAD_SIZE) {
            return false;
        }

        memcpy(
            packet.payload,
            source,
            length
        );

        packet.payloadLength =
            static_cast<uint16_t>(length);

        return true;
    }

    // ============================================================
    // Generic payload getter
    // ============================================================

    static bool getPayload(
        const PacketContainer &packet,
        void *destination,
        size_t expectedLength
    ) {

        if (destination == nullptr) {
            return false;
        }

        if (packet.payloadLength != expectedLength) {
            return false;
        }

        if (expectedLength > MAX_PAYLOAD_SIZE) {
            return false;
        }

        memcpy(
            destination,
            packet.payload,
            expectedLength
        );

        return true;
    }
};

} // namespace SankatNet

#endif