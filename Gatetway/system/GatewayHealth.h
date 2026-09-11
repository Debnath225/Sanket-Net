#ifndef SANKATNET_GATEWAY_HEALTH_H
#define SANKATNET_GATEWAY_HEALTH_H

#include <Arduino.h>


class GatewayHealth
{
public:

    GatewayHealth()
        : _initialized(false),
          _loraFault(false),
          _wifiFault(false),
          _mqttFault(false),
          _securityFault(false),
          _receivedPackets(0),
          _forwardedPackets(0),
          _publishedPackets(0),
          _droppedPackets(0),
          _securityFailures(0),
          _lastUpdate(0)
    {
    }


    /*
     * ========================================================
     * BEGIN
     * ========================================================
     */

    void begin()
    {
        _initialized = true;

        _lastUpdate = millis();
    }


    /*
     * ========================================================
     * UPDATE
     * ========================================================
     */

    void update(
        uint32_t receivedPackets,
        uint32_t forwardedPackets,
        uint32_t publishedPackets,
        uint32_t droppedPackets,
        size_t queueSize)
    {
        if (!_initialized)
        {
            return;
        }

        _receivedPackets = receivedPackets;

        _forwardedPackets = forwardedPackets;

        _publishedPackets = publishedPackets;

        _droppedPackets = droppedPackets;

        _queueSize = queueSize;

        _lastUpdate = millis();
    }


    /*
     * ========================================================
     * LORA FAULT
     * ========================================================
     */

    void raiseLoRaFault()
    {
        _loraFault = true;
    }


    void clearLoRaFault()
    {
        _loraFault = false;
    }


    bool hasLoRaFault() const
    {
        return _loraFault;
    }


    /*
     * ========================================================
     * WIFI FAULT
     * ========================================================
     */

    void raiseWiFiFault()
    {
        _wifiFault = true;
    }


    void clearWiFiFault()
    {
        _wifiFault = false;
    }


    bool hasWiFiFault() const
    {
        return _wifiFault;
    }


    /*
     * ========================================================
     * MQTT FAULT
     * ========================================================
     */

    void raiseMQTTFault()
    {
        _mqttFault = true;
    }


    void clearMQTTFault()
    {
        _mqttFault = false;
    }


    bool hasMQTTFault() const
    {
        return _mqttFault;
    }


    /*
     * ========================================================
     * SECURITY FAULT
     * ========================================================
     */

    void raiseSecurityFault()
    {
        _securityFault = true;
    }


    void clearSecurityFault()
    {
        _securityFault = false;
    }


    bool hasSecurityFault() const
    {
        return _securityFault;
    }


    /*
     * ========================================================
     * SECURITY FAILURE
     * ========================================================
     */

    void recordSecurityFailure()
    {
        _securityFailures++;

        _securityFault = true;
    }


    uint32_t securityFailureCount() const
    {
        return _securityFailures;
    }


    /*
     * ========================================================
     * PACKET STATISTICS
     * ========================================================
     */

    uint32_t receivedPackets() const
    {
        return _receivedPackets;
    }


    uint32_t forwardedPackets() const
    {
        return _forwardedPackets;
    }


    uint32_t publishedPackets() const
    {
        return _publishedPackets;
    }


    uint32_t droppedPackets() const
    {
        return _droppedPackets;
    }


    size_t queueSize() const
    {
        return _queueSize;
    }


    /*
     * ========================================================
     * OVERALL HEALTH
     * ========================================================
     */

    bool isHealthy() const
    {
        return
            !_loraFault &&
            !_securityFault;
    }


    /*
     * ========================================================
     * NETWORK HEALTH
     * ========================================================
     */

    bool networkHealthy() const
    {
        return
            !_wifiFault &&
            !_mqttFault;
    }


    /*
     * ========================================================
     * INITIALIZATION STATUS
     * ========================================================
     */

    bool isInitialized() const
    {
        return _initialized;
    }


    /*
     * ========================================================
     * LAST UPDATE
     * ========================================================
     */

    uint32_t lastUpdate() const
    {
        return _lastUpdate;
    }


private:

    bool _initialized;


    bool _loraFault;

    bool _wifiFault;

    bool _mqttFault;

    bool _securityFault;


    uint32_t _receivedPackets;

    uint32_t _forwardedPackets;

    uint32_t _publishedPackets;

    uint32_t _droppedPackets;

    uint32_t _securityFailures;


    size_t _queueSize = 0;


    uint32_t _lastUpdate;
};


#endif