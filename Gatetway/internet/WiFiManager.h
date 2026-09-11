#ifndef SANKATNET_WIFI_MANAGER_H
#define SANKATNET_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

#include "../config/GatewayConfig.h"


class WiFiManager
{
public:

    /*
     * ========================================================
     * Constructor
     * ========================================================
     */

    WiFiManager()
        : _initialized(false),
          _connected(false),
          _lastConnectionAttempt(0),
          _connectionAttempts(0),
          _successfulConnections(0)
    {
    }


    /*
     * ========================================================
     * BEGIN
     * ========================================================
     *
     * Wi-Fi credentials are intentionally not hard-coded here.
     *
     * Define these securely in your build environment:
     *
     *   SANKATNET_WIFI_SSID
     *   SANKATNET_WIFI_PASSWORD
     *
     * For a prototype, these can alternatively be defined
     * before including this header.
     * ========================================================
     */

    bool begin()
    {
        _initialized = false;
        _connected = false;

        WiFi.mode(WIFI_STA);

        /*
         * Do not run an access point on the gateway.
         *
         * The gateway acts as a station connected to the
         * available Internet network.
         */

        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);


        /*
         * Obtain credentials.
         */

        const char *ssid = getSSID();
        const char *password = getPassword();


        if (ssid == nullptr || strlen(ssid) == 0)
        {
            Serial.println(
                "[WiFi] ERROR: SSID is not configured"
            );

            return false;
        }


        /*
         * Start connection.
         */

        WiFi.begin(ssid, password);

        _lastConnectionAttempt = millis();
        _connectionAttempts++;


        /*
         * Wait only for the initial connection.
         *
         * Runtime reconnects are handled non-blockingly by
         * reconnect().
         */

        uint32_t startTime = millis();

        while (WiFi.status() != WL_CONNECTED)
        {
            if (millis() - startTime >=
                GatewayConfig::WIFI_CONNECT_TIMEOUT_MS)
            {
                Serial.println(
                    "[WiFi] Initial connection timeout"
                );

                _connected = false;

                return false;
            }


            delay(100);
        }


        _connected = true;
        _initialized = true;
        _successfulConnections++;


        printConnectionInfo();

        return true;
    }


    /*
     * ========================================================
     * RECONNECT
     * ========================================================
     *
     * Non-blocking reconnect attempt.
     * ========================================================
     */

    bool reconnect()
    {
        if (!_initialized)
        {
            return false;
        }


        /*
         * Already connected.
         */

        if (WiFi.status() == WL_CONNECTED)
        {
            _connected = true;

            return true;
        }


        _connected = false;


        /*
         * Prevent excessive reconnect attempts.
         */

        if (millis() - _lastConnectionAttempt <
            GatewayConfig::WIFI_RECONNECT_INTERVAL_MS)
        {
            return false;
        }


        _lastConnectionAttempt = millis();

        _connectionAttempts++;


        Serial.println(
            "[WiFi] Attempting reconnect..."
        );


        WiFi.disconnect(false);

        delay(50);


        const char *ssid = getSSID();
        const char *password = getPassword();


        if (ssid == nullptr || strlen(ssid) == 0)
        {
            Serial.println(
                "[WiFi] SSID unavailable"
            );

            return false;
        }


        WiFi.begin(ssid, password);


        /*
         * Do not wait here.
         *
         * The main gateway loop must remain available for
         * LoRa packet reception.
         */

        return false;
    }


    /*
     * ========================================================
     * UPDATE
     * ========================================================
     *
     * Call periodically from the gateway loop.
     * ========================================================
     */

    void update()
    {
        wl_status_t status = WiFi.status();


        if (status == WL_CONNECTED)
        {
            if (!_connected)
            {
                _connected = true;
                _successfulConnections++;

                Serial.println(
                    "[WiFi] Connection restored"
                );

                printConnectionInfo();
            }

            return;
        }


        if (_connected)
        {
            _connected = false;

            Serial.println(
                "[WiFi] Connection lost"
            );
        }
    }


    /*
     * ========================================================
     * IS CONNECTED
     * ========================================================
     */

    bool isConnected()
    {
        update();

        return _connected;
    }


    /*
     * ========================================================
     * INITIALIZED
     * ========================================================
     */

    bool isInitialized() const
    {
        return _initialized;
    }


    /*
     * ========================================================
     * IP ADDRESS
     * ========================================================
     */

    String ipAddress() const
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return String("0.0.0.0");
        }

        return WiFi.localIP().toString();
    }


    /*
     * ========================================================
     * GATEWAY ADDRESS
     * ========================================================
     */

    String gatewayAddress() const
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return String("0.0.0.0");
        }

        return WiFi.gatewayIP().toString();
    }


    /*
     * ========================================================
     * DNS ADDRESS
     * ========================================================
     */

    String dnsAddress() const
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return String("0.0.0.0");
        }

        return WiFi.dnsIP().toString();
    }


    /*
     * ========================================================
     * RSSI
     * ========================================================
     */

    int32_t rssi() const
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return -127;
        }

        return WiFi.RSSI();
    }


    /*
     * ========================================================
     * SSID
     * ========================================================
     */

    String connectedSSID() const
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return String();
        }

        return WiFi.SSID();
    }


    /*
     * ========================================================
     * MAC ADDRESS
     * ========================================================
     */

    String macAddress() const
    {
        return WiFi.macAddress();
    }


    /*
     * ========================================================
     * CONNECTION STATISTICS
     * ========================================================
     */

    uint32_t connectionAttempts() const
    {
        return _connectionAttempts;
    }


    uint32_t successfulConnections() const
    {
        return _successfulConnections;
    }


    /*
     * ========================================================
     * DISCONNECT
     * ========================================================
     */

    void disconnect()
    {
        WiFi.disconnect(false);

        _connected = false;
    }


private:

    bool _initialized;

    bool _connected;

    uint32_t _lastConnectionAttempt;

    uint32_t _connectionAttempts;

    uint32_t _successfulConnections;


    /*
     * ========================================================
     * GET SSID
     * ========================================================
     *
     * Production:
     *
     *   Pass credentials through secure provisioning.
     *
     * Prototype:
     *
     *   Define:
     *
     *   #define SANKATNET_WIFI_SSID "YourWiFi"
     *   #define SANKATNET_WIFI_PASSWORD "YourPassword"
     *
     * before including this file.
     * ========================================================
     */

    const char *getSSID() const
    {
#ifdef SANKATNET_WIFI_SSID

        return SANKATNET_WIFI_SSID;

#else

        return "";

#endif
    }


    /*
     * ========================================================
     * GET PASSWORD
     * ========================================================
     */

    const char *getPassword() const
    {
#ifdef SANKATNET_WIFI_PASSWORD

        return SANKATNET_WIFI_PASSWORD;

#else

        return "";

#endif
    }


    /*
     * ========================================================
     * PRINT CONNECTION INFORMATION
     * ========================================================
     */

    void printConnectionInfo() const
    {
        Serial.println(
            "------------------------------------------"
        );

        Serial.println("[WiFi] Connected");

        Serial.print("[WiFi] SSID    : ");
        Serial.println(WiFi.SSID());

        Serial.print("[WiFi] IP      : ");
        Serial.println(WiFi.localIP());

        Serial.print("[WiFi] Gateway : ");
        Serial.println(WiFi.gatewayIP());

        Serial.print("[WiFi] RSSI    : ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");

        Serial.print("[WiFi] MAC     : ");
        Serial.println(WiFi.macAddress());

        Serial.println(
            "------------------------------------------"
        );
    }
};


#endif