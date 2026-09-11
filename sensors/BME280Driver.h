#ifndef SANKATNET_BME280_DRIVER_H
#define SANKATNET_BME280_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

#include "../config/PinConfig.h"
#include "../config/Calibration.h"


namespace SankatNet {


class BME280Driver {

public:

    // ========================================================
    // CONSTRUCTOR
    // ========================================================

    BME280Driver() = default;


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin()
    {
        /*
         * Try the configured address first.
         */
        if (sensor.begin(Pins::I2CAddress::BME280, &Wire)) {

            initialized = true;

            /*
             * Normal weather/environment measurement mode.
             *
             * The BME280 performs continuous measurements while
             * the rest of the firmware remains non-blocking.
             */
            sensor.setSampling(
                Adafruit_BME280::MODE_NORMAL,

                // Temperature
                Adafruit_BME280::SAMPLING_X2,

                // Pressure
                Adafruit_BME280::SAMPLING_X4,

                // Humidity
                Adafruit_BME280::SAMPLING_X2,

                // Filter
                Adafruit_BME280::FILTER_X4,

                // Standby
                Adafruit_BME280::STANDBY_MS_1000
            );

            return true;
        }

        initialized = false;

        return false;
    }


    // ========================================================
    // STATUS
    // ========================================================

    bool isInitialized() const
    {
        return initialized;
    }


    // ========================================================
    // READ SENSOR
    // ========================================================

    bool read(
        float &temperatureC,
        float &humidityPercent,
        float &pressurePa
    )
    {
        if (!initialized) {
            return false;
        }


        float temperature =
            sensor.readTemperature();

        float humidity =
            sensor.readHumidity();

        float pressure =
            sensor.readPressure();


        /*
         * Validate readings before passing them further into
         * the system.
         */
        if (!isfinite(temperature)) {
            return false;
        }

        if (!isfinite(humidity)) {
            return false;
        }

        if (!isfinite(pressure)) {
            return false;
        }


        /*
         * Basic physical sanity limits.
         *
         * These prevent obviously corrupted I2C readings from
         * entering the risk engine.
         */
        if (
            temperature < -50.0f ||
            temperature > 85.0f
        ) {
            return false;
        }

        if (
            humidity < 0.0f ||
            humidity > 100.0f
        ) {
            return false;
        }

        if (
            pressure < 30000.0f ||
            pressure > 120000.0f
        ) {
            return false;
        }


        /*
         * Apply project-level calibration offsets.
         */
        temperatureC =
            Calibration::BME280::
            calibrateTemperature(
                temperature
            );

        humidityPercent =
            Calibration::BME280::
            calibrateHumidity(
                humidity
            );

        pressurePa =
            Calibration::BME280::
            calibratePressure(
                pressure
            );


        lastTemperature = temperatureC;
        lastHumidity = humidityPercent;
        lastPressure = pressurePa;

        lastReadTime = millis();

        return true;
    }


    // ========================================================
    // LAST VALUES
    // ========================================================

    float temperature() const
    {
        return lastTemperature;
    }


    float humidity() const
    {
        return lastHumidity;
    }


    float pressure() const
    {
        return lastPressure;
    }


    uint32_t lastReadMillis() const
    {
        return lastReadTime;
    }


private:

    // ========================================================
    // HARDWARE OBJECT
    // ========================================================

    Adafruit_BME280 sensor;


    // ========================================================
    // STATE
    // ========================================================

    bool initialized = false;


    float lastTemperature = NAN;
    float lastHumidity = NAN;
    float lastPressure = NAN;

    uint32_t lastReadTime = 0;
};


} // namespace SankatNet


#endif