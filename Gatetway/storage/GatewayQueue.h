#ifndef SANKATNET_GATEWAY_QUEUE_H
#define SANKATNET_GATEWAY_QUEUE_H

#include <Arduino.h>

#include "../config/GatewayConfig.h"
#include "../network/Packet.h"


class GatewayQueue
{
public:

    struct QueueItem
    {
        PacketContainer packet;

        uint32_t queuedAt;

        bool used;
    };


    GatewayQueue()
        : _head(0),
          _tail(0),
          _count(0),
          _initialized(false),
          _enqueued(0),
          _dequeued(0),
          _dropped(0)
    {
        for (size_t i = 0;
             i < GatewayConfig::GATEWAY_QUEUE_SIZE;
             i++)
        {
            _items[i].used = false;
            _items[i].queuedAt = 0;
        }
    }


    /*
     * ========================================================
     * BEGIN
     * ========================================================
     */

    void begin()
    {
        _head = 0;
        _tail = 0;
        _count = 0;

        _enqueued = 0;
        _dequeued = 0;
        _dropped = 0;

        for (size_t i = 0;
             i < GatewayConfig::GATEWAY_QUEUE_SIZE;
             i++)
        {
            _items[i].used = false;
            _items[i].queuedAt = 0;
        }

        _initialized = true;
    }


    /*
     * ========================================================
     * ENQUEUE
     * ========================================================
     */

    bool enqueue(
        const PacketContainer &packet)
    {
        if (!_initialized)
        {
            return false;
        }


        /*
         * Queue full.
         *
         * Critical packets should preferably be
         * handled immediately by Gateway.ino.
         */

        if (_count >= GatewayConfig::GATEWAY_QUEUE_SIZE)
        {
            _dropped++;

            return false;
        }


        _items[_tail].packet = packet;

        _items[_tail].queuedAt =
            millis();

        _items[_tail].used = true;


        _tail++;

        if (_tail >= GatewayConfig::GATEWAY_QUEUE_SIZE)
        {
            _tail = 0;
        }


        _count++;

        _enqueued++;

        return true;
    }


    /*
     * ========================================================
     * PEEK
     * ========================================================
     *
     * Returns the oldest queued packet without
     * removing it.
     */

    bool peek(
        PacketContainer &packet)
    {
        if (!_initialized ||
            _count == 0)
        {
            return false;
        }


        removeExpired();


        if (_count == 0)
        {
            return false;
        }


        packet =
            _items[_head].packet;


        return true;
    }


    /*
     * ========================================================
     * DEQUEUE
     * ========================================================
     */

    bool dequeue(
        PacketContainer &packet)
    {
        if (!_initialized ||
            _count == 0)
        {
            return false;
        }


        removeExpired();


        if (_count == 0)
        {
            return false;
        }


        packet =
            _items[_head].packet;


        _items[_head].used = false;

        _items[_head].queuedAt = 0;


        _head++;

        if (_head >= GatewayConfig::GATEWAY_QUEUE_SIZE)
        {
            _head = 0;
        }


        _count--;

        _dequeued++;

        return true;
    }


    /*
     * ========================================================
     * REMOVE EXPIRED PACKETS
     * ========================================================
     */

    void removeExpired()
    {
        if (!_initialized)
        {
            return;
        }


        uint32_t now = millis();


        while (_count > 0)
        {
            uint32_t queuedAt =
                _items[_head].queuedAt;


            uint32_t age =
                now - queuedAt;


            if (age <=
                GatewayConfig::QUEUE_MAX_PACKET_AGE_MS)
            {
                break;
            }


            /*
             * Old packet can no longer be trusted
             * to represent current gateway state.
             */

            _items[_head].used = false;
            _items[_head].queuedAt = 0;


            _head++;

            if (_head >= GatewayConfig::GATEWAY_QUEUE_SIZE)
            {
                _head = 0;
            }


            _count--;

            _dropped++;
        }
    }


    /*
     * ========================================================
     * EMPTY
     * ========================================================
     */

    bool empty()
    {
        removeExpired();

        return (_count == 0);
    }


    /*
     * ========================================================
     * FULL
     * ========================================================
     */

    bool full() const
    {
        return
            (_count >=
             GatewayConfig::GATEWAY_QUEUE_SIZE);
    }


    /*
     * ========================================================
     * SIZE
     * ========================================================
     */

    size_t size()
    {
        removeExpired();

        return _count;
    }


    /*
     * ========================================================
     * CAPACITY
     * ========================================================
     */

    size_t capacity() const
    {
        return GatewayConfig::GATEWAY_QUEUE_SIZE;
    }


    /*
     * ========================================================
     * CLEAR
     * ========================================================
     */

    void clear()
    {
        _head = 0;
        _tail = 0;
        _count = 0;


        for (size_t i = 0;
             i < GatewayConfig::GATEWAY_QUEUE_SIZE;
             i++)
        {
            _items[i].used = false;
            _items[i].queuedAt = 0;
        }
    }


    /*
     * ========================================================
     * STATISTICS
     * ========================================================
     */

    uint32_t enqueuedCount() const
    {
        return _enqueued;
    }


    uint32_t dequeuedCount() const
    {
        return _dequeued;
    }


    uint32_t droppedCount() const
    {
        return _dropped;
    }


private:

    QueueItem
        _items[GatewayConfig::GATEWAY_QUEUE_SIZE];


    size_t _head;

    size_t _tail;

    size_t _count;


    bool _initialized;


    uint32_t _enqueued;

    uint32_t _dequeued;

    uint32_t _dropped;
};


#endif