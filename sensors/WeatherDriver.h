#ifndef SANKATNET_WEATHER_DRIVER_H
#define SANKATNET_WEATHER_DRIVER_H

#include <Arduino.h>

#include "../config/PinConfig.h"
#include "../config/Calibration.h"

namespace SankatNet {

class WeatherDriver {
public:

    WeatherDriver()
        : initialized(false),
          rainPin(Pins::RAIN_GAUGE),
          windPin(Pins::ANEMOMETER),
          rainPulses(0),
          windPulses(0),
          lastUpdateMillis(0),
          rainfallMM(0.0f),
          windSpeedMS(0.0f),
          updateCount(0),
          failureCount(0) {

        instance = this;
    }

    // ------------------------------------------------------------
    // Initialize rain gauge and anemometer
    // ------------------------------------------------------------
    bool begin(
        uint8_t rainGaugePin = Pins::RAIN_GAUGE,
        uint8_t anemometerPin = Pins::ANEMOMETER
    ) {

        rainPin = rainGaugePin;
        windPin = anemometerPin;

        instance = this;

        pinMode(rainPin, INPUT_PULLUP);
        pinMode(windPin, INPUT_PULLUP);

        rainPulses = 0;
        windPulses = 0;

        /*
         * Both sensors are pulse-output devices.
         *
         * FALLING edge is used because the reed switch / pulse
         * output normally pulls the signal LOW when activated.
         */
        if (digitalPinToInterrupt(rainPin) != NOT_AN_INTERRUPT) {
            attachInterrupt(
                digitalPinToInterrupt(rainPin),
                rainISR,
                FALLING
            );
        } else {
            failureCount++;
            initialized = false;
            return false;
        }

        if (digitalPinToInterrupt(windPin) != NOT_AN_INTERRUPT) {
            attachInterrupt(
                digitalPinToInterrupt(windPin),
                windISR,
                FALLING
            );
        } else {
            detachInterrupt(digitalPinToInterrupt(rainPin));
            failureCount++;
            initialized = false;
            return false;
        }

        lastUpdateMillis = millis();
        initialized = true;

        return true;
    }

    // ------------------------------------------------------------
    // Process accumulated pulses
    //
    // Call periodically from loop().
    // Default processing interval: 5 seconds.
    // ------------------------------------------------------------
    bool update() {

        if (!initialized) {
            return false;
        }

        const uint32_t now = millis();

        if ((now - lastUpdateMillis) < UPDATE_INTERVAL_MS) {
            return true;
        }

        uint32_t rainCount;
        uint32_t windCount;

        /*
         * Copy and clear ISR counters atomically.
         */
        noInterrupts();

        rainCount = rainPulses;
        windCount = windPulses;

        rainPulses = 0;
        windPulses = 0;

        interrupts();

        /*
         * Calculate the measurement interval.
         */
        const float intervalSeconds =
            static_cast<float>(now - lastUpdateMillis) / 1000.0f;

        if (intervalSeconds <= 0.0f) {
            return false;
        }

        // --------------------------------------------------------
        // Rainfall
        // --------------------------------------------------------
        rainfallMM =
            static_cast<float>(rainCount) *
            Calibration::RainGauge::MM_PER_PULSE;

        // --------------------------------------------------------
        // Wind speed
        //
        // wind frequency = pulses / measurement time
        // wind speed = frequency × calibration factor
        // --------------------------------------------------------
        const float frequencyHz =
            static_cast<float>(windCount) / intervalSeconds;

        windSpeedMS =
            Calibration::Anemometer::speedFromFrequency(frequencyHz);

        lastUpdateMillis = now;
        updateCount++;

        return true;
    }

    // ------------------------------------------------------------
    // Initialization status
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    // ------------------------------------------------------------
    // Rainfall during the most recent measurement interval
    // ------------------------------------------------------------
    float rainfall() const {
        return rainfallMM;
    }

    float rainfallMMValue() const {
        return rainfallMM;
    }

    // ------------------------------------------------------------
    // Wind speed during the most recent measurement interval
    // ------------------------------------------------------------
    float windSpeed() const {
        return windSpeedMS;
    }

    float windSpeedMSValue() const {
        return windSpeedMS;
    }

    // ------------------------------------------------------------
    // Raw pulse counters
    //
    // These represent pulses accumulated since the previous
    // update() processing cycle.
    // ------------------------------------------------------------
    uint32_t rainPulseCount() const {

        uint32_t value;

        noInterrupts();
        value = rainPulses;
        interrupts();

        return value;
    }

    uint32_t windPulseCount() const {

        uint32_t value;

        noInterrupts();
        value = windPulses;
        interrupts();

        return value;
    }

    // ------------------------------------------------------------
    // Data age
    // ------------------------------------------------------------
    uint32_t age() const {

        if (lastUpdateMillis == 0) {
            return UINT32_MAX;
        }

        return millis() - lastUpdateMillis;
    }

    uint32_t lastUpdate() const {
        return lastUpdateMillis;
    }

    // ------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------
    uint32_t updateCountTotal() const {
        return updateCount;
    }

    uint32_t failureCountTotal() const {
        return failureCount;
    }

    // ------------------------------------------------------------
    // Reset statistics
    // ------------------------------------------------------------
    void resetStatistics() {

        updateCount = 0;
        failureCount = 0;
    }

private:

    // ------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------
    static constexpr uint32_t UPDATE_INTERVAL_MS = 5000;

    // ------------------------------------------------------------
    // Hardware
    // ------------------------------------------------------------
    uint8_t rainPin;
    uint8_t windPin;

    // ------------------------------------------------------------
    // ISR counters
    //
    // volatile because these variables are modified inside
    // interrupt service routines.
    // ------------------------------------------------------------
    volatile uint32_t rainPulses;
    volatile uint32_t windPulses;

    // ------------------------------------------------------------
    // Processed values
    // ------------------------------------------------------------
    uint32_t lastUpdateMillis;

    float rainfallMM;
    float windSpeedMS;

    // ------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------
    uint32_t updateCount;
    uint32_t failureCount;

    bool initialized;

    // ------------------------------------------------------------
    // Singleton instance used by ESP32 ISR callbacks
    // ------------------------------------------------------------
    static WeatherDriver *instance;

    // ------------------------------------------------------------
    // Rain gauge ISR
    // ------------------------------------------------------------
    static void IRAM_ATTR rainISR() {

        if (instance != nullptr) {
            instance->rainPulses++;
        }
    }

    // ------------------------------------------------------------
    // Anemometer ISR
    // ------------------------------------------------------------
    static void IRAM_ATTR windISR() {

        if (instance != nullptr) {
            instance->windPulses++;
        }
    }
};

// ------------------------------------------------------------
// Static member definition
//
// C++17 inline variable prevents multiple-definition problems
// when this header is included by multiple compilation units.
// ------------------------------------------------------------
inline WeatherDriver *WeatherDriver::instance = nullptr;

} // namespace SankatNet

#endif