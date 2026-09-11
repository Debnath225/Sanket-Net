#ifndef SANKATNET_LORA_DRIVER_H
#define SANKATNET_LORA_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#include "../config/PinConfig.h"
#include "../config/NodeConfig.h"

namespace SankatNet {

class LoRaDriver {
public:

    struct RadioStats {
        uint32_t txPackets;
        uint32_t rxPackets;
        uint32_t txFailures;
        uint32_t rxOverflows;
        uint32_t invalidPackets;
    };

    LoRaDriver()
        : initialized(false),
          receiving(false),
          lastRSSI(0),
          lastSNR(0.0f),
          lastPacketSize(0),
          stats{} {}

    // ------------------------------------------------------------
    // Initialize SX1278 LoRa radio
    // ------------------------------------------------------------
    bool begin() {

        SPI.begin(
            Pins::LORA_SCK,
            Pins::LORA_MISO,
            Pins::LORA_MOSI,
            Pins::LORA_NSS
        );

        LoRa.setSPI(SPI);
        LoRa.setPins(
            Pins::LORA_NSS,
            Pins::LORA_RST,
            Pins::LORA_DIO0
        );

        /*
         * Start radio at the configured frequency.
         */
        if (!LoRa.begin(LoRaConfig::FREQUENCY)) {
            initialized = false;
            return false;
        }

        // --------------------------------------------------------
        // LoRa modem configuration
        // --------------------------------------------------------
        LoRa.setSignalBandwidth(
            LoRaConfig::BANDWIDTH
        );

        LoRa.setSpreadingFactor(
            LoRaConfig::SPREADING_FACTOR
        );

        LoRa.setCodingRate4(
            LoRaConfig::CODING_RATE
        );

        LoRa.setSyncWord(
            LoRaConfig::SYNC_WORD
        );

        LoRa.setTxPower(
            LoRaConfig::TX_POWER_DBM
        );

        LoRa.setPreambleLength(
            LoRaConfig::PREAMBLE_LENGTH
        );

        if (LoRaConfig::ENABLE_CRC) {
            LoRa.enableCrc();
        } else {
            LoRa.disableCrc();
        }

        /*
         * Put the radio into receive mode immediately after
         * configuration.
         */
        LoRa.receive();

        initialized = true;
        receiving = true;

        return true;
    }

    // ------------------------------------------------------------
    // Transmit raw binary packet
    //
    // Encryption/authentication is performed by CryptoManager,
    // not by this low-level radio driver.
    // ------------------------------------------------------------
    bool transmit(
        const uint8_t *data,
        size_t length
    ) {

        if (!initialized || data == nullptr) {
            return false;
        }

        if (length == 0 ||
            length > MAX_LORA_PACKET_SIZE) {
            stats.invalidPackets++;
            return false;
        }

        /*
         * Stop receive mode before transmission.
         */
        LoRa.idle();
        receiving = false;

        int result = LoRa.beginPacket();

        if (result == 0) {
            stats.txFailures++;

            LoRa.receive();
            receiving = true;

            return false;
        }

        size_t written =
            LoRa.write(data, length);

        if (written != length) {
            stats.txFailures++;

            LoRa.endPacket();

            LoRa.receive();
            receiving = true;

            return false;
        }

        /*
         * endPacket() waits for the LoRa transmission to finish.
         *
         * This operation is relatively short compared with sensor
         * conversion operations, but should still not be called
         * repeatedly in a tight loop.
         */
        result = LoRa.endPacket();

        if (result == 0) {
            stats.txFailures++;

            LoRa.receive();
            receiving = true;

            return false;
        }

        stats.txPackets++;

        /*
         * Return to receive mode immediately.
         */
        LoRa.receive();
        receiving = true;

        return true;
    }

    // ------------------------------------------------------------
    // Receive packet
    //
    // Returns:
    //   true  = packet received
    //   false = no packet available / invalid arguments
    // ------------------------------------------------------------
    bool receive(
        uint8_t *buffer,
        size_t bufferSize,
        size_t &receivedLength
    ) {

        receivedLength = 0;

        if (!initialized ||
            buffer == nullptr ||
            bufferSize == 0) {
            return false;
        }

        /*
         * parsePacket() checks whether a complete LoRa packet
         * is available without blocking for a new packet.
         */
        int packetSize = LoRa.parsePacket();

        if (packetSize <= 0) {
            return false;
        }

        lastPacketSize =
            static_cast<size_t>(packetSize);

        /*
         * Protect against oversized packets.
         */
        if (static_cast<size_t>(packetSize) > bufferSize ||
            static_cast<size_t>(packetSize) >
                MAX_LORA_PACKET_SIZE) {

            /*
             * Drain the packet so that the radio can return to
             * normal receive operation.
             */
            while (LoRa.available()) {
                LoRa.read();
            }

            stats.rxOverflows++;

            LoRa.receive();
            receiving = true;

            return false;
        }

        size_t index = 0;

        while (LoRa.available() && index < bufferSize) {

            int value = LoRa.read();

            if (value < 0) {
                break;
            }

            buffer[index++] =
                static_cast<uint8_t>(value);
        }

        receivedLength = index;

        lastRSSI = LoRa.packetRssi();
        lastSNR = LoRa.packetSnr();

        if (receivedLength !=
            static_cast<size_t>(packetSize)) {

            stats.rxOverflows++;

            LoRa.receive();
            receiving = true;

            return false;
        }

        stats.rxPackets++;

        LoRa.receive();
        receiving = true;

        return true;
    }

    // ------------------------------------------------------------
    // Explicitly enter receive mode
    // ------------------------------------------------------------
    void startReceive() {

        if (!initialized) {
            return;
        }

        LoRa.receive();
        receiving = true;
    }

    // ------------------------------------------------------------
    // Put radio into standby
    // ------------------------------------------------------------
    void standby() {

        if (!initialized) {
            return;
        }

        LoRa.idle();
        receiving = false;
    }

    // ------------------------------------------------------------
    // Radio state
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    bool isReceiving() const {
        return receiving;
    }

    // ------------------------------------------------------------
    // Last received signal information
    // ------------------------------------------------------------
    int lastRSSIValue() const {
        return lastRSSI;
    }

    float lastSNRValue() const {
        return lastSNR;
    }

    float getSNR() const {
        return lastSNRValue();
    }

    int getRSSI() const {
        return lastRSSIValue();
    }

    size_t lastPacketSizeValue() const {
        return lastPacketSize;
    }

    // ------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------
    const RadioStats &getStats() const {
        return stats;
    }

    uint32_t txCount() const {
        return stats.txPackets;
    }

    uint32_t rxCount() const {
        return stats.rxPackets;
    }

    uint32_t txFailureCount() const {
        return stats.txFailures;
    }

    uint32_t rxOverflowCount() const {
        return stats.rxOverflows;
    }

    uint32_t invalidPacketCount() const {
        return stats.invalidPackets;
    }

    // ------------------------------------------------------------
    // Reset statistics
    // ------------------------------------------------------------
    void resetStatistics() {

        stats = RadioStats{};
    }

private:

    bool initialized;
    bool receiving;

    int lastRSSI;
    float lastSNR;
    size_t lastPacketSize;

    RadioStats stats;
};

} // namespace SankatNet

#endif