#ifndef SANKATNET_NVS_MANAGER_H
#define SANKATNET_NVS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

#include "../config/NodeConfig.h"


namespace SankatNet {


class NVSManager {

public:

    // ========================================================
    // NVS NAMESPACE
    // ========================================================

    static constexpr const char* NAMESPACE =
        "sankatnet";


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin()
    {
        if (initialized) {
            return true;
        }


        if (
            !preferences.begin(
                NAMESPACE,
                false
            )
        ) {

            initialized =
                false;

            return false;
        }


        initialized =
            true;


        /*
         * Increment boot counter every time the node starts.
         */

        bootCounter =
            preferences.getULong(
                KEY_BOOT_COUNTER,
                0
            );

        bootCounter++;

        preferences.putULong(
            KEY_BOOT_COUNTER,
            bootCounter
        );


        /*
         * Load persistent sequence number.
         */

        sequenceNumber =
            preferences.getULong(
                KEY_SEQUENCE,
                0
            );


        /*
         * Configuration version.
         */

        configVersion =
            preferences.getUChar(
                KEY_CONFIG_VERSION,
                1
            );


        return true;
    }


    // ========================================================
    // END
    // ========================================================

    void end()
    {
        if (
            initialized
        ) {

            preferences.end();

            initialized =
                false;
        }
    }


    // ========================================================
    // SEQUENCE NUMBER
    // ========================================================

    uint32_t nextSequence()
    {
        if (
            !initialized
        ) {
            return UINT32_MAX;
        }


        /*
         * Increment before returning.
         *
         * This prevents sequence 0 from being reused after
         * normal initialization.
         */

        if (sequenceNumber == UINT32_MAX) {
            return UINT32_MAX;
        }

        sequenceNumber++;


        /*
         * Persist immediately.
         *
         * This provides the strongest replay-protection
         * guarantee, at the cost of additional NVS writes.
         */

        if (preferences.putULong(
            KEY_SEQUENCE,
            sequenceNumber
        ) == 0) {
            return UINT32_MAX;
        }


        return sequenceNumber;
    }


    uint32_t getSequence() const
    {
        return sequenceNumber;
    }


    bool setSequence(
        uint32_t value
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        sequenceNumber =
            value;


        return (
            preferences.putULong(
                KEY_SEQUENCE,
                sequenceNumber
            ) > 0
        );
    }


    // ========================================================
    // NODE ID
    // ========================================================

    uint16_t getNodeId() const
    {
        /*
         * NODE_ID is compile-time configuration.
         *
         * This method exists so the application layer has a
         * single storage abstraction.
         */

        return NODE_ID;
    }


    // ========================================================
    // BOOT COUNTER
    // ========================================================

    uint32_t getBootCounter() const
    {
        return bootCounter;
    }


    // ========================================================
    // CONFIGURATION VERSION
    // ========================================================

    uint8_t getConfigVersion() const
    {
        return configVersion;
    }


    bool setConfigVersion(
        uint8_t version
    )
    {
        if (
            !initialized
        ) {
            return false;
        }


        configVersion =
            version;


        return (
            preferences.putUChar(
                KEY_CONFIG_VERSION,
                version
            ) > 0
        );
    }


    // ========================================================
    // GENERIC UINT32
    // ========================================================

    bool putUInt32(
        const char* key,
        uint32_t value
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return false;
        }


        return (
            preferences.putULong(
                key,
                value
            ) > 0
        );
    }


    uint32_t getUInt32(
        const char* key,
        uint32_t defaultValue = 0
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return defaultValue;
        }


        return preferences.getULong(
            key,
            defaultValue
        );
    }


    // ========================================================
    // GENERIC UINT16
    // ========================================================

    bool putUInt16(
        const char* key,
        uint16_t value
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return false;
        }


        return (
            preferences.putUShort(
                key,
                value
            ) > 0
        );
    }


    uint16_t getUInt16(
        const char* key,
        uint16_t defaultValue = 0
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return defaultValue;
        }


        return preferences.getUShort(
            key,
            defaultValue
        );
    }


    // ========================================================
    // GENERIC UINT8
    // ========================================================

    bool putUInt8(
        const char* key,
        uint8_t value
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return false;
        }


        return (
            preferences.putUChar(
                key,
                value
            ) > 0
        );
    }


    uint8_t getUInt8(
        const char* key,
        uint8_t defaultValue = 0
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return defaultValue;
        }


        return preferences.getUChar(
            key,
            defaultValue
        );
    }


    // ========================================================
    // FLOAT
    // ========================================================

    bool putFloat(
        const char* key,
        float value
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return false;
        }


        return (
            preferences.putFloat(
                key,
                value
            ) > 0
        );
    }


    float getFloat(
        const char* key,
        float defaultValue = 0.0f
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return defaultValue;
        }


        return preferences.getFloat(
            key,
            defaultValue
        );
    }


    // ========================================================
    // STRING
    // ========================================================

    bool putString(
        const char* key,
        const String& value
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return false;
        }


        return (
            preferences.putString(
                key,
                value
            ) > 0
        );
    }


    String getString(
        const char* key,
        const String& defaultValue = ""
    )
    {
        if (
            !initialized ||
            key == nullptr
        ) {
            return defaultValue;
        }


        return preferences.getString(
            key,
            defaultValue
        );
    }


    // ========================================================
    // CLEAR CONFIGURATION
    // ========================================================

    bool clearAll()
    {
        if (
            !initialized
        ) {
            return false;
        }


        /*
         * Remove all Sankat-Net NVS keys.
         */

        return (
            preferences.clear()
        );
    }


    // ========================================================
    // STATUS
    // ========================================================

    bool isInitialized() const
    {
        return initialized;
    }


private:

    // ========================================================
    // KEYS
    // ========================================================

    static constexpr const char* KEY_SEQUENCE =
        "sequence";

    static constexpr const char* KEY_BOOT_COUNTER =
        "boot_count";

    static constexpr const char* KEY_CONFIG_VERSION =
        "cfg_ver";


    // ========================================================
    // ESP32 PREFERENCES
    // ========================================================

    Preferences preferences;


    // ========================================================
    // RUNTIME STATE
    // ========================================================

    bool initialized =
        false;

    uint32_t sequenceNumber =
        0;

    uint32_t bootCounter =
        0;

    uint8_t configVersion =
        1;
};


} // namespace SankatNet


#endif