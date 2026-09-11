#ifndef SANKATNET_PACKET_QUEUE_H
#define SANKATNET_PACKET_QUEUE_H

#include <Arduino.h>

#include "../config/NodeConfig.h"
#include "../network/Packet.h"


namespace SankatNet {


class PacketQueue {

public:

    // ========================================================
    // QUEUE ENTRY
    // ========================================================

    struct QueueEntry {

        PacketContainer packet;

        uint32_t queuedAt =
            0;

        uint8_t retryCount =
            0;

        bool active =
            false;
    };


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
    // ENQUEUE
    // ========================================================

    bool enqueue(
        const PacketContainer& packet
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        /*
         * Prevent the same packet from being queued twice.
         */

        if (
            contains(
                packet.header.sourceNode,
                packet.header.sequence
            )
        ) {

            return true;
        }


        int index =
            findFreeEntry();


        /*
         * If queue is full, make room according to priority.
         */

        if (
            index < 0
        ) {

            index =
                makeRoomFor(packet);

            if (
                index < 0
            ) {
                droppedFull++;
                return false;
            }
        }


        queue[index].packet =
            packet;

        queue[index].queuedAt =
            millis();

        queue[index].retryCount =
            0;

        queue[index].active =
            true;


        queueLength++;

        enqueued++;

        return true;
    }


    // ========================================================
    // DEQUEUE
    // ========================================================

    bool dequeue(
        PacketContainer& packet
    )
    {
        int index =
            findBestPacket();


        if (
            index < 0
        ) {
            return false;
        }


        packet =
            queue[index].packet;


        queue[index].active =
            false;


        if (
            queueLength > 0
        ) {
            queueLength--;
        }


        dequeued++;

        return true;
    }


    // ========================================================
    // PEEK
    // ========================================================

    bool peek(
        PacketContainer& packet
    ) const
    {
        int index =
            findBestPacket();


        if (
            index < 0
        ) {
            return false;
        }


        packet =
            queue[index].packet;


        return true;
    }


    // ========================================================
    // REMOVE SPECIFIC PACKET
    // ========================================================

    bool remove(
        uint16_t sourceNode,
        uint32_t sequence
    )
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            if (
                queue[i].packet.header.sourceNode ==
                    sourceNode &&
                queue[i].packet.header.sequence ==
                    sequence
            ) {

                queue[i].active =
                    false;


                if (
                    queueLength > 0
                ) {
                    queueLength--;
                }


                return true;
            }
        }


        return false;
    }


    // ========================================================
    // CHECK CONTAINS
    // ========================================================

    bool contains(
        uint16_t sourceNode,
        uint32_t sequence
    ) const
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            if (
                queue[i].packet.header.sourceNode ==
                    sourceNode &&
                queue[i].packet.header.sequence ==
                    sequence
            ) {

                return true;
            }
        }


        return false;
    }


    // ========================================================
    // EXPIRE OLD PACKETS
    // ========================================================

    void cleanup()
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
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            if (
                now -
                queue[i].queuedAt >
                SystemLimits::MAX_QUEUE_AGE_MS
            ) {

                queue[i].active =
                    false;


                if (
                    queueLength > 0
                ) {
                    queueLength--;
                }


                expired++;
            }
        }
    }


    // ========================================================
    // RETRY COUNT
    // ========================================================

    bool incrementRetry(
        uint16_t sourceNode,
        uint32_t sequence
    )
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            if (
                queue[i].packet.header.sourceNode ==
                    sourceNode &&
                queue[i].packet.header.sequence ==
                    sequence
            ) {

                if (
                    queue[i].retryCount <
                    255
                ) {
                    queue[i].retryCount++;
                }

                return true;
            }
        }


        return false;
    }


    uint8_t getRetryCount(
        uint16_t sourceNode,
        uint32_t sequence
    ) const
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            if (
                queue[i].packet.header.sourceNode ==
                    sourceNode &&
                queue[i].packet.header.sequence ==
                    sequence
            ) {

                return queue[i].retryCount;
            }
        }


        return 0;
    }


    // ========================================================
    // QUEUE INFORMATION
    // ========================================================

    uint8_t size() const
    {
        return queueLength;
    }


    uint8_t capacity() const
    {
        return MAX_QUEUE_SIZE;
    }


    bool isEmpty() const
    {
        return queueLength == 0;
    }


    bool isFull() const
    {
        return queueLength >= MAX_QUEUE_SIZE;
    }


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t getEnqueuedCount() const
    {
        return enqueued;
    }


    uint32_t getDequeuedCount() const
    {
        return dequeued;
    }


    uint32_t getDroppedCount() const
    {
        return droppedFull;
    }


    uint32_t getExpiredCount() const
    {
        return expired;
    }


    // ========================================================
    // CLEAR
    // ========================================================

    void clear()
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            queue[i] =
                QueueEntry{};
        }


        queueLength =
            0;
    }


private:

    // ========================================================
    // QUEUE SIZE
    // ========================================================

    static constexpr uint8_t MAX_QUEUE_SIZE =
        MeshConfig::PACKET_QUEUE_SIZE;


    // ========================================================
    // STORAGE
    // ========================================================

    QueueEntry queue[
        MAX_QUEUE_SIZE
    ];


    uint8_t queueLength =
        0;


    bool initialized =
        false;


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t enqueued =
        0;

    uint32_t dequeued =
        0;

    uint32_t droppedFull =
        0;

    uint32_t expired =
        0;


    // ========================================================
    // FIND FREE ENTRY
    // ========================================================

    int findFreeEntry() const
    {
        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                return i;
            }
        }


        return -1;
    }


    // ========================================================
    // FIND BEST PACKET
    // ========================================================

    int findBestPacket() const
    {
        int bestIndex =
            -1;

        uint8_t bestPriority =
            0;


        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            uint8_t priority =
                getPacketPriority(
                    queue[i].packet
                );


            /*
             * Higher priority packets are transmitted first.
             */

            if (
                bestIndex < 0 ||
                priority > bestPriority
            ) {

                bestIndex =
                    i;

                bestPriority =
                    priority;
            }
        }


        return bestIndex;
    }


    // ========================================================
    // PRIORITY
    // ========================================================

    uint8_t getPacketPriority(
        const PacketContainer& packet
    ) const
    {
        /*
         * Critical packets always receive the highest queue
         * priority.
         */

        if (
            PacketFlags::FLAG_CRITICAL &
            packet.header.flags
        ) {
            return PRIORITY_CRITICAL;
        }


        if (
            packet.header.packetType ==
            PKT_ALERT
        ) {
            return PRIORITY_HIGH;
        }


        if (
            packet.header.packetType ==
            PKT_CONFIG
        ) {
            return PRIORITY_HIGH;
        }


        return PRIORITY_NORMAL;
    }


    // ========================================================
    // MAKE ROOM
    // ========================================================

    int makeRoomFor(
        const PacketContainer& incoming
    )
    {
        uint8_t incomingPriority =
            getPacketPriority(
                incoming
            );


        int lowestIndex =
            -1;

        uint8_t lowestPriority =
            255;


        /*
         * Find the lowest-priority packet.
         */

        for (
            uint8_t i = 0;
            i < MAX_QUEUE_SIZE;
            i++
        ) {

            if (
                !queue[i].active
            ) {
                continue;
            }


            uint8_t priority =
                getPacketPriority(
                    queue[i].packet
                );


            if (
                priority <
                lowestPriority
            ) {

                lowestPriority =
                    priority;

                lowestIndex =
                    i;
            }
        }


        /*
         * Do not throw away an equally or more important
         * packet just to store the incoming packet.
         */

        if (
            lowestIndex < 0 ||
            incomingPriority <= lowestPriority
        ) {

            return -1;
        }


        queue[lowestIndex].active =
            false;


        if (
            queueLength > 0
        ) {
            queueLength--;
        }


        droppedPriority++;

        return lowestIndex;
    }


    uint32_t droppedPriority =
        0;
};


} // namespace SankatNet


#endif