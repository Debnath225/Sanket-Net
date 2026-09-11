#ifndef SANKATNET_BATTERY_MANAGER_H
#define SANKATNET_BATTERY_MANAGER_H

#include <Arduino.h>

#include "../config/PinConfig.h"
#include "../config/NodeConfig.h"
#include "../config/Calibration.h"


namespace SankatNet {


class BatteryManager {

public:

    // ========================================================
    // BATTERY STATE
    // ========================================================

    enum class BatteryState : uint8_t {

        UNKNOWN = 0,

        NORMAL = 1,

        LOW = 2,

        CRITICAL = 3
    };


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin()
    {
        pinMode(
            Pins::BATTERY_ADC,
            INPUT
        );


        lastCheck =
            0;

        voltageMV =
            0;

        percentage =
            0;

        state =
            BatteryState::UNKNOWN;

        initialized =
            true;

        return true;
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


        if (
            now -
            lastCheck <
            BatteryConfig::CHECK_INTERVAL_MS
        ) {
            return;
        }


        lastCheck =
            now;


        readBattery();
    }


    // ========================================================
    // FORCE UPDATE
    // ========================================================

    bool readNow()
    {
        if (
            !initialized
        ) {
            return false;
        }


        return readBattery();
    }


    // ========================================================
    // VOLTAGE
    // ========================================================

    uint16_t getVoltageMV() const
    {
        return voltageMV;
    }


    float getVoltage() const
    {
        return (
            static_cast<float>(
                voltageMV
            ) / 1000.0f
        );
    }


    // ========================================================
    // PERCENTAGE
    // ========================================================

    uint8_t getPercentage() const
    {
        return percentage;
    }


    // ========================================================
    // STATE
    // ========================================================

    BatteryState getState() const
    {
        return state;
    }


    bool isNormal() const
    {
        return (
            state ==
            BatteryState::NORMAL
        );
    }


    bool isLow() const
    {
        return (
            state ==
            BatteryState::LOW
        );
    }


    bool isCritical() const
    {
        return (
            state ==
            BatteryState::CRITICAL
        );
    }


    // ========================================================
    // LAST UPDATE
    // ========================================================

    uint32_t getLastUpdateTime() const
    {
        return lastCheck;
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
    // READ BATTERY
    // ========================================================

    bool readBattery()
    {
        /*
         * Average several ADC readings to reduce measurement
         * noise.
         */

        constexpr uint8_t SAMPLE_COUNT =
            16;


        uint32_t total =
            0;


        for (
            uint8_t i = 0;
            i < SAMPLE_COUNT;
            i++
        ) {

            total +=
                analogRead(
                    Pins::BATTERY_ADC
                );

            delayMicroseconds(200);
        }


        uint16_t raw =
            static_cast<uint16_t>(
                total / SAMPLE_COUNT
            );


        /*
         * Convert ADC reading to battery voltage using the
         * calibration module.
         */

        uint16_t measuredMV =
            Calibration::Battery::millivoltsFromADC(
                raw
            );


        /*
         * Reject obviously invalid measurements.
         */

        if (
            measuredMV == 0 ||
            measuredMV > 6000
        ) {

            return false;
        }


        voltageMV =
            measuredMV;


        percentage =
            calculatePercentage(
                voltageMV
            );


        updateState(
            voltageMV
        );


        return true;
    }


    // ========================================================
    // BATTERY PERCENTAGE
    // ========================================================

    uint8_t calculatePercentage(
        uint16_t mv
    ) const
    {
        /*
         * Generic Li-ion/LiPo approximation.
         *
         * This is NOT a fuel-gauge measurement.
         *
         * The actual percentage depends on battery chemistry,
         * load, temperature, discharge curve and BMS.
         */

        constexpr uint16_t EMPTY_MV =
            3000;

        constexpr uint16_t FULL_MV =
            4200;


        if (
            mv <= EMPTY_MV
        ) {
            return 0;
        }


        if (
            mv >= FULL_MV
        ) {
            return 100;
        }


        uint32_t percentageValue =
            (
                static_cast<uint32_t>(
                    mv - EMPTY_MV
                ) *
                100UL
            ) /
            (
                FULL_MV -
                EMPTY_MV
            );


        if (
            percentageValue > 100
        ) {
            percentageValue =
                100;
        }


        return static_cast<uint8_t>(
            percentageValue
        );
    }


    // ========================================================
    // UPDATE STATE
    // ========================================================

    void updateState(
        uint16_t mv
    )
    {
        if (
            mv <=
            BatteryConfig::CRITICAL_BATTERY_MV
        ) {

            state =
                BatteryState::CRITICAL;

            return;
        }


        if (
            mv <=
            BatteryConfig::LOW_BATTERY_MV
        ) {

            state =
                BatteryState::LOW;

            return;
        }


        state =
            BatteryState::NORMAL;
    }


    // ========================================================
    // MEMBERS
    // ========================================================

    bool initialized =
        false;


    uint16_t voltageMV =
        0;


    uint8_t percentage =
        0;


    BatteryState state =
        BatteryState::UNKNOWN;


    uint32_t lastCheck =
        0;
};


} // namespace SankatNet


#endif