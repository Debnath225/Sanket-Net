#ifndef SANKATNET_REPLAY_PROTECTION_H
#define SANKATNET_REPLAY_PROTECTION_H

#include <Arduino.h>

#include "Packet.h"
#include "../config/NodeConfig.h"


namespace SankatNet {


class ReplayProtection {

public:

    // ========================================================
    // CONFIGURATION
    // ========================================================

    static constexpr uint8_t CACHE_SIZE =
        MeshConfig::DUPLICATE_CACHE_SIZE;

    /*
     * Maximum acceptable packet age.
     *
     * Packets older than this are rejected unless their
     * timestamp is zero (for packets that do not use time).
     */
    static constexpr uint32_t MAX_PACKET_AGE_MS =
        10UL * 60UL * 1000UL;


    // ========================================================
    // INITIALIZATION
    // ========================================================

    void begin()
    {
        clear();

        initialized =
            true;
    }


    // ========================================================
    // CHECK PACKET
    // ========================================================

    bool accept(
        const PacketHeader& header
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        // ----------------------------------------------------
        // Basic header validation
        // ----------------------------------------------------

        if (
            header.magic != PACKET_MAGIC
        ) {
            rejectedInvalid++;
            return false;
        }


        if (
            header.version != PACKET_VERSION
        ) {
            rejectedInvalid++;
            return false;
        }


        if (
            header.sourceNode == NODE_ID
        ) {
            rejectedOwnPacket++;
            return false;
        }


        // ----------------------------------------------------
        // Check duplicate cache
        // ----------------------------------------------------

        if (
            isDuplicate(
                header.sourceNode,
                header.sequence
            )
        ) {

            rejectedDuplicate++;

            /*
             * Remember that this packet was seen.
             * This is important because a repeated packet must
             * never be processed as a new packet.
             */

            return false;
        }


        // ----------------------------------------------------
        // Timestamp validation
        // ----------------------------------------------------

        if (
            header.timestamp != 0
        ) {

            uint32_t now =
                millis();

            /*
             * The packet timestamp is expected to be based on
             * the node's uptime/system time.
             *
             * Unsynchronized clocks are therefore handled
             * conservatively.
             */

            if (
                isTimestampTooOld(
                    header.timestamp,
                    now
                )
            ) {

                rejectedExpired++;

                return false;
            }
        }


        // ----------------------------------------------------
        // Accept packet
        // ----------------------------------------------------

        remember(
            header.sourceNode,
            header.sequence
        );

        accepted++;

        return true;
    }


    // ========================================================
    // DUPLICATE CHECK ONLY
    // ========================================================

    bool isDuplicate(
        uint16_t sourceNode,
        uint32_t sequence
    ) const
    {
        for (
            uint8_t i = 0;
            i < CACHE_SIZE;
            i++
        ) {

            if (
                !cache[i].valid
            ) {
                continue;
            }


            if (
                cache[i].sourceNode ==
                    sourceNode &&
                cache[i].sequence ==
                    sequence
            ) {

                return true;
            }
        }


        return false;
    }


    // ========================================================
    // REMEMBER PACKET
    // ========================================================

    bool remember(
        uint16_t sourceNode,
        uint32_t sequence
    )
    {
        /*
         * If already present, nothing needs to be changed.
         */

        if (
            isDuplicate(
                sourceNode,
                sequence
            )
        ) {
            return true;
        }


        int index =
            findReplacementEntry();


        if (
            index < 0
        ) {
            return false;
        }


        cache[index].sourceNode =
            sourceNode;

        cache[index].sequence =
            sequence;

        cache[index].receivedTime =
            millis();

        cache[index].valid =
            true;


        return true;
    }


    // ========================================================
    // TIMESTAMP VALIDATION
    // ========================================================

    bool isTimestampTooOld(
        uint32_t packetTimestamp,
        uint32_t currentTime
    ) const
    {
        /*
         * Handle normal unsigned millis() wraparound safely.
         */

        uint32_t age =
            currentTime -
            packetTimestamp;


        return (
            age >
            MAX_PACKET_AGE_MS
        );
    }


    // ========================================================
    // REMOVE OLD CACHE ENTRIES
    // ========================================================

    void cleanup()
    {
        uint32_t now =
            millis();


        for (
            uint8_t i = 0;
            i < CACHE_SIZE;
            i++
        ) {

            if (
                !cache[i].valid
            ) {
                continue;
            }


            if (
                now -
                cache[i].receivedTime >
                MAX_PACKET_AGE_MS
            ) {

                cache[i].valid =
                    false;
            }
        }
    }


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t getAcceptedCount() const
    {
        return accepted;
    }


    uint32_t getRejectedDuplicateCount() const
    {
        return rejectedDuplicate;
    }


    uint32_t getRejectedExpiredCount() const
    {
        return rejectedExpired;
    }


    uint32_t getRejectedInvalidCount() const
    {
        return rejectedInvalid;
    }


    uint32_t getRejectedOwnPacketCount() const
    {
        return rejectedOwnPacket;
    }


    // ========================================================
    // CLEAR
    // ========================================================

    void clear()
    {
        for (
            uint8_t i = 0;
            i < CACHE_SIZE;
            i++
        ) {

            cache[i] =
                CacheEntry{};
        }


        cacheWriteIndex =
            0;


        accepted =
            0;

        rejectedDuplicate =
            0;

        rejectedExpired =
            0;

        rejectedInvalid =
            0;

        rejectedOwnPacket =
            0;
    }


private:

    // ========================================================
    // CACHE ENTRY
    // ========================================================

    struct CacheEntry {

        uint16_t sourceNode =
            0;

        uint32_t sequence =
            0;

        uint32_t receivedTime =
            0;

        bool valid =
            false;
    };


    // ========================================================
    // CACHE
    // ========================================================

    CacheEntry cache[
        CACHE_SIZE
    ];


    uint8_t cacheWriteIndex =
        0;


    // ========================================================
    // STATE
    // ========================================================

    bool initialized =
        false;


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t accepted =
        0;

    uint32_t rejectedDuplicate =
        0;

    uint32_t rejectedExpired =
        0;

    uint32_t rejectedInvalid =
        0;

    uint32_t rejectedOwnPacket =
        0;


    // ========================================================
    // FIND REPLACEMENT ENTRY
    // ========================================================

    int findReplacementEntry()
    {
        /*
         * First look for an unused entry.
         */

        for (
            uint8_t i = 0;
            i < CACHE_SIZE;
            i++
        ) {

            if (
                !cache[i].valid
            ) {
                return i;
            }
        }


        /*
         * Cache is full.
         *
         * Replace entries in round-robin order.
         */

        int index =
            cacheWriteIndex;


        cacheWriteIndex++;

        if (
            cacheWriteIndex >=
            CACHE_SIZE
        ) {

            cacheWriteIndex =
                0;
        }


        return index;
    }
};


} // namespace SankatNet


#endif