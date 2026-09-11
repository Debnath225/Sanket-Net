#ifndef SANKATNET_ACK_MANAGER_H
#define SANKATNET_ACK_MANAGER_H

#include <Arduino.h>

#include "Packet.h"
#include "LoRaDriver.h"
#include "../config/NodeConfig.h"


namespace SankatNet {


class AckManager {

public:

    // ========================================================
    // ACK ENTRY
    // ========================================================

    struct PendingAck {

        bool active = false;

        uint16_t destination = 0;

        uint32_t sequence = 0;

        uint8_t retryCount = 0;

        uint32_t lastTransmitTime = 0;

        uint32_t createdTime = 0;

        PacketContainer packet{};
    };


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin(
        LoRaDriver* radio
    )
    {
        lora = radio;

        clear();

        initialized =
            (lora != nullptr);

        return initialized;
    }


    // ========================================================
    // REGISTER ACK
    // ========================================================

    bool registerPacket(
        uint16_t destination,
        uint32_t sequence,
        const PacketContainer& packet
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        /*
         * Do not register the same packet twice.
         */

        if (
            isPending(
                destination,
                sequence
            )
        ) {
            return true;
        }


        int index =
            findFreeEntry();


        if (
            index < 0
        ) {
            return false;
        }


        pending[index].active =
            true;

        pending[index].destination =
            destination;

        pending[index].sequence =
            sequence;

        pending[index].retryCount =
            0;

        pending[index].lastTransmitTime =
            millis();

        pending[index].createdTime =
            millis();

        pending[index].packet = packet;


        return true;
    }

    bool getRetryPacket(PacketContainer& packet) const
    {
        if (!retryRequired) {
            return false;
        }

        for (uint8_t i = 0; i < MAX_PENDING_ACKS; i++) {
            if (pending[i].active &&
                pending[i].destination == retryDestination &&
                pending[i].sequence == retrySequence) {
                packet = pending[i].packet;
                return true;
            }
        }

        return false;
    }


    // ========================================================
    // PROCESS ACK
    // ========================================================

    bool processAck(
        const AckPayload& ack
    )
    {
        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            if (
                !pending[i].active
            ) {
                continue;
            }


            if (
                pending[i].sequence ==
                ack.acknowledgedSequence
            ) {

                if (ack.sourceNode != pending[i].destination) {
                    continue;
                }

                /*
                 * ACK is valid only when it refers to the
                 * expected source packet.
                 */

                if (
                    ack.status ==
                    ACK_OK
                ) {

                    pending[i].active =
                        false;

                    successfulAcks++;

                    return true;
                }


                /*
                 * A rejected ACK also completes the
                 * transaction. The upper layer can decide
                 * whether to queue the packet again.
                 */

                if (
                    ack.status ==
                    ACK_REJECTED ||
                    ack.status ==
                    ACK_INVALID ||
                    ack.status ==
                    ACK_NO_ROUTE
                ) {

                    pending[i].active =
                        false;

                    failedAcks++;

                    return false;
                }


                /*
                 * Duplicate packet:
                 * delivery may already have happened.
                 */

                if (
                    ack.status ==
                    ACK_DUPLICATE
                ) {

                    pending[i].active =
                        false;

                    duplicateAcks++;

                    return true;
                }
            }
        }


        return false;
    }


    // ========================================================
    // UPDATE
    // ========================================================

    void update()
    {
        if (
            !initialized
        ) {
            return;
        }


        uint32_t now =
            millis();


        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            if (
                !pending[i].active
            ) {
                continue;
            }


            /*
             * Wait for ACK timeout.
             */

            if (
                now -
                pending[i].lastTransmitTime <
                Timing::ACK_TIMEOUT_MS
            ) {
                continue;
            }


            // ------------------------------------------------
            // Retry
            // ------------------------------------------------

            if (
                pending[i].retryCount <
                Timing::MAX_RETRIES
            ) {

                pending[i].retryCount++;

                pending[i].lastTransmitTime =
                    now;

                retryCount++;

                /*
                 * The actual packet retransmission is handled
                 * by the owner of the packet. AckManager only
                 * reports that a retry is required.
                 */

                retryRequired =
                    true;

                retryDestination =
                    pending[i].destination;

                retrySequence =
                    pending[i].sequence;

                return;
            }


            // ------------------------------------------------
            // Delivery failure
            // ------------------------------------------------

            pending[i].active =
                false;

            deliveryFailures++;

            failureDestination =
                pending[i].destination;

            failureSequence =
                pending[i].sequence;

            deliveryFailure =
                true;
        }
    }


    // ========================================================
    // RETRY REQUEST
    // ========================================================

    bool retryNeeded() const
    {
        return retryRequired;
    }


    uint16_t getRetryDestination() const
    {
        return retryDestination;
    }


    uint32_t getRetrySequence() const
    {
        return retrySequence;
    }


    void clearRetryRequest()
    {
        retryRequired =
            false;
    }


    // ========================================================
    // DELIVERY FAILURE
    // ========================================================

    bool hasDeliveryFailure() const
    {
        return deliveryFailure;
    }


    uint16_t getFailureDestination() const
    {
        return failureDestination;
    }


    uint32_t getFailureSequence() const
    {
        return failureSequence;
    }


    void clearDeliveryFailure()
    {
        deliveryFailure =
            false;
    }


    // ========================================================
    // PENDING STATUS
    // ========================================================

    bool isPending(
        uint16_t destination,
        uint32_t sequence
    ) const
    {
        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            if (
                pending[i].active &&
                pending[i].destination ==
                    destination &&
                pending[i].sequence ==
                    sequence
            ) {

                return true;
            }
        }


        return false;
    }


    bool hasPendingPackets() const
    {
        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            if (
                pending[i].active
            ) {
                return true;
            }
        }


        return false;
    }


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t getSuccessfulAcks() const
    {
        return successfulAcks;
    }


    uint32_t getFailedAcks() const
    {
        return failedAcks;
    }


    uint32_t getDuplicateAcks() const
    {
        return duplicateAcks;
    }


    uint32_t getRetryCount() const
    {
        return retryCount;
    }


    uint32_t getDeliveryFailures() const
    {
        return deliveryFailures;
    }


    // ========================================================
    // CLEAR
    // ========================================================

    void clear()
    {
        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            pending[i] =
                PendingAck{};
        }


        retryRequired =
            false;

        deliveryFailure =
            false;
    }


private:

    // ========================================================
    // LIMITS
    // ========================================================

    static constexpr uint8_t MAX_PENDING_ACKS =
        8;


    // ========================================================
    // RADIO
    // ========================================================

    LoRaDriver* lora =
        nullptr;


    bool initialized =
        false;


    // ========================================================
    // ACK TABLE
    // ========================================================

    PendingAck pending[
        MAX_PENDING_ACKS
    ];


    // ========================================================
    // RETRY STATE
    // ========================================================

    bool retryRequired =
        false;

    uint16_t retryDestination =
        0;

    uint32_t retrySequence =
        0;


    // ========================================================
    // FAILURE STATE
    // ========================================================

    bool deliveryFailure =
        false;

    uint16_t failureDestination =
        0;

    uint32_t failureSequence =
        0;


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t successfulAcks =
        0;

    uint32_t failedAcks =
        0;

    uint32_t duplicateAcks =
        0;

    uint32_t retryCount =
        0;

    uint32_t deliveryFailures =
        0;


    // ========================================================
    // FIND FREE ENTRY
    // ========================================================

    int findFreeEntry() const
    {
        for (
            uint8_t i = 0;
            i < MAX_PENDING_ACKS;
            i++
        ) {

            if (
                !pending[i].active
            ) {

                return i;
            }
        }


        return -1;
    }
};


} // namespace SankatNet


#endif