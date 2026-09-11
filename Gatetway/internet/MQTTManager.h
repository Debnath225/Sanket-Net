#ifndef SANKATNET_MQTT_MANAGER_H
#define SANKATNET_MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "../config/GatewayConfig.h"


class MQTTManager
{
public:

    MQTTManager()
        : _client(_secureClient),
          _initialized(false),
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
     * MQTT uses TLS.
     *
     * Broker credentials and CA certificate are supplied
     * through compile-time configuration.
     *
     * Required definitions:
     *
     *   SANKATNET_MQTT_BROKER
     *   SANKATNET_MQTT_PORT
     *   SANKATNET_MQTT_USERNAME
     *   SANKATNET_MQTT_PASSWORD
     *
     * For production, use certificate validation rather than
     * insecure TLS.
     * ========================================================
     */

    bool begin()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println(
                "[MQTT] Wi-Fi not connected"
            );

            return false;
        }


#ifndef SANKATNET_MQTT_BROKER

        Serial.println(
            "[MQTT] ERROR: Broker is not configured"
        );

        return false;

#else

        /*
         * ----------------------------------------------------
         * TLS configuration
         * ----------------------------------------------------
         *
         * The certificate can be supplied using:
         *
         *   SANKATNET_MQTT_ROOT_CA
         *
         * If no CA is supplied, TLS certificate verification
         * is intentionally NOT enabled.
         *
         * That mode is suitable only for development/testing.
         * ----------------------------------------------------
         */

#ifdef SANKATNET_MQTT_ROOT_CA

        _secureClient.setCACert(
            SANKATNET_MQTT_ROOT_CA
        );

#else

        Serial.println(
            "[MQTT] ERROR: Root CA is required for production TLS"
        );

        return false;

#endif


        /*
         * MQTT broker configuration.
         */

        _client.setServer(
            SANKATNET_MQTT_BROKER,
            GatewayConfig::MQTT_PORT
        );


        /*
         * MQTT packet buffer.
         */

        _client.setBufferSize(
            GatewayConfig::MQTT_MAX_PAYLOAD_SIZE
        );


        /*
         * Keep MQTT callbacks lightweight.
         */

        _client.setKeepAlive(
            GatewayConfig::MQTT_KEEP_ALIVE_SECONDS
        );


        _initialized = true;

        return connect();

#endif
    }


    /*
     * ========================================================
     * CONNECT
     * ========================================================
     */

    bool connect()
    {
        if (!_initialized)
        {
            return false;
        }


        if (WiFi.status() != WL_CONNECTED)
        {
            _connected = false;

            return false;
        }


        if (_client.connected())
        {
            _connected = true;

            return true;
        }


        /*
         * Prevent repeated rapid connection attempts.
         */

        if (millis() - _lastConnectionAttempt <
            GatewayConfig::MQTT_RECONNECT_INTERVAL_MS)
        {
            return false;
        }


        _lastConnectionAttempt = millis();

        _connectionAttempts++;


        Serial.println(
            "[MQTT] Connecting to broker..."
        );


        String clientId =
            createClientId();


#ifdef SANKATNET_MQTT_USERNAME

#ifdef SANKATNET_MQTT_PASSWORD

        bool result = _client.connect(
            clientId.c_str(),
            SANKATNET_MQTT_USERNAME,
            SANKATNET_MQTT_PASSWORD
        );

#else

        bool result = _client.connect(
            clientId.c_str()
        );

#endif

#else

        bool result = _client.connect(
            clientId.c_str()
        );

#endif


        if (!result)
        {
            _connected = false;

            Serial.print(
                "[MQTT] Connection failed, state="
            );

            Serial.println(
                _client.state()
            );

            return false;
        }


        _connected = true;
        _successfulConnections++;


        Serial.println(
            "[MQTT] Connected"
        );


        /*
         * Subscribe to backend → gateway command/config
         * channel.
         */

        subscribeCommandTopic();


        /*
         * Publish retained gateway status.
         */

        publishGatewayOnline();


        return true;
    }


    /*
     * ========================================================
     * RECONNECT
     * ========================================================
     */

    bool reconnect()
    {
        if (!_initialized)
        {
            return false;
        }


        if (WiFi.status() != WL_CONNECTED)
        {
            _connected = false;

            return false;
        }


        if (_client.connected())
        {
            _connected = true;

            return true;
        }


        _connected = false;

        return connect();
    }


    /*
     * ========================================================
     * LOOP
     * ========================================================
     */

    void loop()
    {
        if (!_initialized)
        {
            return;
        }


        if (WiFi.status() != WL_CONNECTED)
        {
            _connected = false;

            return;
        }


        if (!_client.connected())
        {
            _connected = false;

            return;
        }


        _connected = _client.loop();
    }


    /*
     * ========================================================
     * IS CONNECTED
     * ========================================================
     */

    bool isConnected()
    {
        if (!_initialized)
        {
            return false;
        }


        if (WiFi.status() != WL_CONNECTED)
        {
            _connected = false;

            return false;
        }


        _connected = _client.connected();

        return _connected;
    }


    /*
     * ========================================================
     * PUBLISH
     * ========================================================
     */

    bool publish(
        const char *topic,
        const uint8_t *payload,
        size_t length,
        uint8_t qos = GatewayConfig::MQTT_QOS_TELEMETRY,
        bool retained = GatewayConfig::MQTT_RETAIN_TELEMETRY)
    {
        if (!isConnected())
        {
            return false;
        }


        if (topic == nullptr ||
            payload == nullptr ||
            length == 0)
        {
            return false;
        }


        if (length >
            GatewayConfig::MQTT_MAX_PAYLOAD_SIZE)
        {
            Serial.println(
                "[MQTT] Payload too large"
            );

            return false;
        }


        /*
         * PubSubClient supports QoS 0 and QoS 1 depending on
         * the publish API/version.
         *
         * For this project, all critical gateway publishing
         * uses the standard publish path.
         */

        bool result =
            _client.publish(
                topic,
                payload,
                length,
                retained
            );


        /*
         * QoS is handled by the broker/client configuration.
         *
         * PubSubClient's basic publish() path is used here
         * intentionally to keep the gateway lightweight.
         */

        (void)qos;


        if (!result)
        {
            Serial.println(
                "[MQTT] Publish failed"
            );

            _connected = _client.connected();

            return false;
        }


        return true;
    }


    /*
     * ========================================================
     * PUBLISH STRING
     * ========================================================
     */

    bool publish(
        const char *topic,
        const String &payload,
        uint8_t qos = GatewayConfig::MQTT_QOS_TELEMETRY,
        bool retained = GatewayConfig::MQTT_RETAIN_TELEMETRY)
    {
        return publish(
            topic,
            reinterpret_cast<const uint8_t *>(
                payload.c_str()
            ),
            payload.length(),
            qos,
            retained
        );
    }


    /*
     * ========================================================
     * SUBSCRIBE
     * ========================================================
     */

    bool subscribe(
        const char *topic,
        uint8_t qos = 0)
    {
        if (!isConnected() ||
            topic == nullptr)
        {
            return false;
        }


        /*
         * PubSubClient uses QoS 0 subscription by default.
         */

        bool result =
            _client.subscribe(topic, qos);


        if (!result)
        {
            Serial.print(
                "[MQTT] Subscribe failed: "
            );

            Serial.println(topic);
        }


        return result;
    }


    /*
     * ========================================================
     * CALLBACK
     * ========================================================
     *
     * The application can register a callback for commands
     * arriving from the backend.
     * ========================================================
     */

    typedef void (*MessageCallback)(
        const char *topic,
        const uint8_t *payload,
        size_t length
    );


    void setCallback(
        MessageCallback callback)
    {
        _callback = callback;

        _client.setCallback(
            mqttCallback
        );

        _activeInstance = this;
    }


    /*
     * ========================================================
     * LAST ERROR
     * ========================================================
     */

    int lastState() const
    {
        return _client.state();
    }


    /*
     * ========================================================
     * STATISTICS
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


private:

    WiFiClientSecure _secureClient;

    PubSubClient _client;

    bool _initialized;

    bool _connected;

    uint32_t _lastConnectionAttempt;

    uint32_t _connectionAttempts;

    uint32_t _successfulConnections;

    MessageCallback _callback = nullptr;


    /*
     * PubSubClient callback is static, therefore a single
     * active MQTT manager instance is used.
     */

    static MQTTManager *_activeInstance;


    /*
     * ========================================================
     * CLIENT ID
     * ========================================================
     */

    String createClientId()
    {
        String clientId =
            "SankatNet-Gateway-";

        clientId +=
            String(
                GatewayConfig::GATEWAY_ID,
                HEX
            );

        clientId += "-";

        clientId +=
            String(
                (uint32_t)ESP.getEfuseMac(),
                HEX
            );

        return clientId;
    }


    /*
     * ========================================================
     * COMMAND SUBSCRIPTION
     * ========================================================
     */

    void subscribeCommandTopic()
    {
        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_COMMAND
            );

        topic += "/";

        topic += String(
            GatewayConfig::GATEWAY_ID
        );


        subscribe(
            topic.c_str(),
            1
        );
    }


    /*
     * ========================================================
     * GATEWAY ONLINE
     * ========================================================
     */

    void publishGatewayOnline()
    {
        String topic =
            String(
                GatewayConfig::MQTT_TOPIC_GATEWAY_STATUS
            );

        String payload =
            "{\"gateway_id\":65535,"
            "\"status\":\"online\"}";


        publish(
            topic.c_str(),
            payload,
            GatewayConfig::MQTT_QOS_GATEWAY,
            GatewayConfig::MQTT_RETAIN_GATEWAY_STATUS
        );
    }


    /*
     * ========================================================
     * MQTT CALLBACK
     * ========================================================
     */

    static void mqttCallback(
        char *topic,
        uint8_t *payload,
        unsigned int length)
    {
        if (_activeInstance == nullptr)
        {
            return;
        }


        if (_activeInstance->_callback == nullptr)
        {
            return;
        }


        _activeInstance->_callback(
            topic,
            payload,
            length
        );
    }
};


MQTTManager *
MQTTManager::_activeInstance = nullptr;


#endif