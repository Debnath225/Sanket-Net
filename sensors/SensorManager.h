#ifndef SANKATNET_SENSOR_MANAGER_H
#define SANKATNET_SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "../config/PinConfig.h"
#include "../config/NodeConfig.h"

#include "BME280Driver.h"
#include "ADS1115Driver.h"
#include "GPSManager.h"
#include "MotionDriver.h"
#include "WeatherDriver.h"


namespace SankatNet {


// ============================================================
// SENSOR STATUS FLAGS
// ============================================================

enum SensorManagerStatusBits : uint32_t {

    SENSOR_STATUS_NONE =
        0,

    SENSOR_STATUS_BME280 =
        (1UL << 0),

    SENSOR_STATUS_MQ2 =
        (1UL << 1),

    SENSOR_STATUS_MQ135 =
        (1UL << 2),

    SENSOR_STATUS_FLAME =
        (1UL << 3),

    SENSOR_STATUS_WATER_LEVEL =
        (1UL << 4),

    SENSOR_STATUS_SOIL_MOISTURE =
        (1UL << 5),

    SENSOR_STATUS_SOIL_TEMPERATURE =
        (1UL << 6),

    SENSOR_STATUS_MPU6050 =
        (1UL << 7),

    SENSOR_STATUS_RAINFALL =
        (1UL << 8),

    SENSOR_STATUS_WIND_SPEED =
        (1UL << 9),

    SENSOR_STATUS_TURBIDITY =
        (1UL << 10),

    SENSOR_STATUS_PH =
        (1UL << 11),

    SENSOR_STATUS_TDS =
        (1UL << 12),

    SENSOR_STATUS_GPS =
        (1UL << 13),

    SENSOR_STATUS_BATTERY =
        (1UL << 14),

    SENSOR_STATUS_LOW_BATTERY =
        (1UL << 15),

    SENSOR_STATUS_CRITICAL_BATTERY =
        (1UL << 16)
};


// ============================================================
// UNIFIED SENSOR DATA
// ============================================================

struct SensorData {

    // --------------------------------------------------------
    // Environment
    // --------------------------------------------------------

    float temperature = 0.0f;

    float humidity = 0.0f;

    float pressure = 0.0f;


    // --------------------------------------------------------
    // Fire / Air Pollution
    // --------------------------------------------------------

    uint16_t mq2 = 0;

    uint16_t mq135 = 0;

    bool flameDetected = false;


    // --------------------------------------------------------
    // Flood / Soil
    // --------------------------------------------------------

    uint16_t waterLevel = 0;

    uint16_t soilMoisture = 0;

    float soilTemperature = 0.0f;


    // --------------------------------------------------------
    // Motion / Seismic
    // --------------------------------------------------------

    float accelerationX = 0.0f;

    float accelerationY = 0.0f;

    float accelerationZ = 0.0f;

    float vibrationMS2 = 0.0f;


    // --------------------------------------------------------
    // Weather
    // --------------------------------------------------------

    float rainfallMM = 0.0f;

    float windSpeedMS = 0.0f;


    // --------------------------------------------------------
    // Water Quality
    // --------------------------------------------------------

    uint16_t turbidity = 0;

    float ph = 0.0f;

    uint16_t tds = 0;


    // --------------------------------------------------------
    // GPS
    // --------------------------------------------------------

    double latitude = 0.0;

    double longitude = 0.0;

    float altitude = 0.0f;

    uint8_t satellites = 0;

    float hdop = 0.0f;

    bool gpsFix = false;


    // --------------------------------------------------------
    // Battery
    // --------------------------------------------------------

    uint16_t battery_mV = 0;


    // --------------------------------------------------------
    // Health
    // --------------------------------------------------------

    uint32_t sensorStatus =
        SENSOR_STATUS_NONE;


    // --------------------------------------------------------
    // Timestamp
    // --------------------------------------------------------

    uint32_t timestamp = 0;
};


// ============================================================
// SENSOR MANAGER
// ============================================================

class SensorManager {

public:

    // ========================================================
    // BEGIN
    // ========================================================

    bool begin()
    {
        bool success = true;


        // ----------------------------------------------------
        // I2C
        // ----------------------------------------------------

        Wire.begin(
            Pins::I2C_SDA,
            Pins::I2C_SCL
        );

        Wire.setClock(
            Pins::I2C_FREQUENCY
        );


        // ----------------------------------------------------
        // BME280
        // ----------------------------------------------------

        if (
            bme280.begin()
        ) {

            sensorData.sensorStatus |=
                SENSOR_STATUS_BME280;

        }
        else {

            Serial.println(
                "[SENSOR] BME280 not detected"
            );

            success = false;
        }


        // ----------------------------------------------------
        // ADS1115
        // ----------------------------------------------------

        if (
            ads1115.begin()
        ) {

            /*
             * The individual channels are checked during
             * update().
             */

        }
        else {

            Serial.println(
                "[SENSOR] ADS1115 not detected"
            );

            success = false;
        }


        // ----------------------------------------------------
        // GPS
        // ----------------------------------------------------

        if (
            gps.begin(
                Pins::GPS_RX,
                Pins::GPS_TX,
                Pins::GPS_BAUDRATE
            )
        ) {

            /*
             * GPS can initialize even before obtaining a fix.
             */

        }
        else {

            Serial.println(
                "[SENSOR] GPS initialization failed"
            );

            success = false;
        }


        // ----------------------------------------------------
        // MPU6050
        // ----------------------------------------------------

        if (
            motion.begin(
                &Wire,
                Pins::I2CAddress::MPU6050
            )
        ) {

            sensorData.sensorStatus |=
                SENSOR_STATUS_MPU6050;

        }
        else {

            Serial.println(
                "[SENSOR] MPU6050 not detected"
            );

            success = false;
        }


        // ----------------------------------------------------
        // DS18B20
        // ----------------------------------------------------

        oneWire.begin(
            Pins::DS18B20
        );

        ds18b20.begin();


        if (
            ds18b20.getDeviceCount() > 0
        ) {

            ds18b20Ready =
                true;

            sensorData.sensorStatus |=
                SENSOR_STATUS_SOIL_TEMPERATURE;

        }
        else {

            Serial.println(
                "[SENSOR] DS18B20 not detected"
            );

            ds18b20Ready =
                false;

            success = false;
        }


        // ----------------------------------------------------
        // WEATHER
        // ----------------------------------------------------

        if (
            weather.begin(
                Pins::RAIN_GAUGE,
                Pins::ANEMOMETER
            )
        ) {

            sensorData.sensorStatus |=
                SENSOR_STATUS_RAINFALL |
                SENSOR_STATUS_WIND_SPEED;

        }
        else {

            Serial.println(
                "[SENSOR] Weather sensors initialization failed"
            );

            success = false;
        }


        // ----------------------------------------------------
        // FLAME SENSOR
        // ----------------------------------------------------

        pinMode(
            Pins::FLAME_SENSOR,
            INPUT
        );


        // ----------------------------------------------------
        // INITIAL BATTERY ADC
        // ----------------------------------------------------

        pinMode(
            Pins::BATTERY_ADC,
            INPUT
        );


        initialized =
            true;


        return success;
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


        // ----------------------------------------------------
        // GPS
        // ----------------------------------------------------

        gps.update();


        updateGPS();


        // ----------------------------------------------------
        // WEATHER
        // ----------------------------------------------------

        weather.update();


        updateWeather();


        // ----------------------------------------------------
        // MOTION
        // ----------------------------------------------------

        if (
            now - lastMotionSample >=
            SensorTiming::MOTION_SAMPLE_INTERVAL_MS
        ) {

            updateMotion();

            lastMotionSample =
                now;
        }


        // ----------------------------------------------------
        // NORMAL SENSOR SAMPLE
        // ----------------------------------------------------

        if (
            now - lastSensorSample >=
            SensorTiming::SENSOR_SAMPLE_INTERVAL_MS
        ) {

            updateEnvironment();

            updateFireSensors();

            updateFloodSoilSensors();

            updateWaterQuality();

            updateSoilTemperature();

            updateFlame();

            lastSensorSample =
                now;
        }


        sensorData.timestamp =
            now;
    }


    // ========================================================
    // GET DATA
    // ========================================================

    const SensorData& getData() const
    {
        return sensorData;
    }


    // ========================================================
    // STATUS
    // ========================================================

    uint32_t getStatus() const
    {
        return sensorData.sensorStatus;
    }


    // ========================================================
    // HEALTH
    // ========================================================

    bool sensorHealthy() const
    {
        /*
         * BME280 is the minimum environmental sensor.
         *
         * LoRa/GPS health is handled by their respective
         * system modules.
         */

        return (
            sensorData.sensorStatus &
            SENSOR_STATUS_BME280
        );
    }


    bool isInitialized() const
    {
        return initialized;
    }


    // ========================================================
    // INDIVIDUAL DATA ACCESS
    // ========================================================

    float getTemperature() const
    {
        return sensorData.temperature;
    }


    float getHumidity() const
    {
        return sensorData.humidity;
    }


    float getPressure() const
    {
        return sensorData.pressure;
    }


    uint16_t getMQ2() const
    {
        return sensorData.mq2;
    }


    uint16_t getMQ135() const
    {
        return sensorData.mq135;
    }


    bool isFlameDetected() const
    {
        return sensorData.flameDetected;
    }


    uint16_t getWaterLevel() const
    {
        return sensorData.waterLevel;
    }


    uint16_t getSoilMoisture() const
    {
        return sensorData.soilMoisture;
    }


    float getSoilTemperature() const
    {
        return sensorData.soilTemperature;
    }


    float getVibration() const
    {
        return sensorData.vibrationMS2;
    }


    float getRainfall() const
    {
        return sensorData.rainfallMM;
    }


    float getWindSpeed() const
    {
        return sensorData.windSpeedMS;
    }


    uint16_t getTurbidity() const
    {
        return sensorData.turbidity;
    }


    float getPH() const
    {
        return sensorData.ph;
    }


    uint16_t getTDS() const
    {
        return sensorData.tds;
    }


    bool hasGPSFix() const
    {
        return sensorData.gpsFix;
    }


    double getLatitude() const
    {
        return sensorData.latitude;
    }


    double getLongitude() const
    {
        return sensorData.longitude;
    }


    float getAltitude() const
    {
        return sensorData.altitude;
    }


private:

    // ========================================================
    // DRIVER OBJECTS
    // ========================================================

    BME280Driver bme280;

    ADS1115Driver ads1115;

    GPSManager gps;

    MotionDriver motion;

    WeatherDriver weather;


    // ========================================================
    // DS18B20
    // ========================================================

    OneWire oneWire{
        Pins::DS18B20
    };

    DallasTemperature ds18b20{
        &oneWire
    };

    bool ds18b20Ready =
        false;


    // ========================================================
    // STATE
    // ========================================================

    SensorData sensorData;

    bool initialized =
        false;


    uint32_t lastSensorSample =
        0;

    uint32_t lastMotionSample =
        0;


    // ========================================================
    // ENVIRONMENT
    // ========================================================

    void updateEnvironment()
    {
        float temperature;
        float humidity;
        float pressure;


        if (
            bme280.read(
                temperature,
                humidity,
                pressure
            )
        ) {

            sensorData.temperature =
                temperature;

            sensorData.humidity =
                humidity;

            sensorData.pressure =
                pressure;


            sensorData.sensorStatus |=
                SENSOR_STATUS_BME280;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_BME280;
        }
    }


    // ========================================================
    // FIRE / AIR
    // ========================================================

    void updateFireSensors()
    {
        uint16_t value;


        if (
            ads1115.readMQ2(
                value
            )
        ) {

            sensorData.mq2 =
                value;

            sensorData.sensorStatus |=
                SENSOR_STATUS_MQ2;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_MQ2;
        }


        if (
            ads1115.readMQ135(
                value
            )
        ) {

            sensorData.mq135 =
                value;

            sensorData.sensorStatus |=
                SENSOR_STATUS_MQ135;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_MQ135;
        }
    }


    // ========================================================
    // FLAME
    // ========================================================

    void updateFlame()
    {
        /*
         * Many flame modules are active LOW.
         *
         * This assumes:
         *
         * LOW  = flame detected
         * HIGH = normal
         *
         * Verify this against the actual module.
         */

        int state =
            digitalRead(
                Pins::FLAME_SENSOR
            );


        sensorData.flameDetected =
            (state == LOW);


        sensorData.sensorStatus |=
            SENSOR_STATUS_FLAME;
    }


    // ========================================================
    // FLOOD / SOIL
    // ========================================================

    void updateFloodSoilSensors()
    {
        uint16_t value;


        // ----------------------------------------------------
        // Water level
        // ----------------------------------------------------

        if (
            ads1115.readWaterLevel(
                value
            )
        ) {

            sensorData.waterLevel =
                value;

            sensorData.sensorStatus |=
                SENSOR_STATUS_WATER_LEVEL;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_WATER_LEVEL;
        }


        // ----------------------------------------------------
        // Soil moisture
        // ----------------------------------------------------

        if (
            ads1115.readSoilMoisture(
                value
            )
        ) {

            sensorData.soilMoisture =
                value;

            sensorData.sensorStatus |=
                SENSOR_STATUS_SOIL_MOISTURE;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_SOIL_MOISTURE;
        }
    }


    // ========================================================
    // SOIL TEMPERATURE
    // ========================================================

    void updateSoilTemperature()
    {
        if (
            !ds18b20Ready
        ) {
            return;
        }


        /*
         * Non-blocking conversion strategy:
         *
         * requestTemperatures() is used here because this
         * sensor is sampled only every 5 seconds.
         *
         * For a strict non-blocking production implementation,
         * conversion request and readback should be separated.
         */

        ds18b20.requestTemperatures();


        float temperature =
            ds18b20.getTempCByIndex(0);


        if (
            temperature !=
            DEVICE_DISCONNECTED_C &&
            temperature >= -55.0f &&
            temperature <= 125.0f
        ) {

            sensorData.soilTemperature =
                temperature;

            sensorData.sensorStatus |=
                SENSOR_STATUS_SOIL_TEMPERATURE;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_SOIL_TEMPERATURE;
        }
    }


    // ========================================================
    // WATER QUALITY
    // ========================================================

    void updateWaterQuality()
    {
        uint16_t rawValue;


        // ----------------------------------------------------
        // Turbidity
        // ----------------------------------------------------

        if (
            ads1115.readTurbidity(
                rawValue
            )
        ) {

            sensorData.turbidity =
                rawValue;

            sensorData.sensorStatus |=
                SENSOR_STATUS_TURBIDITY;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_TURBIDITY;
        }


        // ----------------------------------------------------
        // pH
        // ----------------------------------------------------

        float phValue;


        if (
            ads1115.readPH(
                phValue
            )
        ) {

            sensorData.ph =
                phValue;

            sensorData.sensorStatus |=
                SENSOR_STATUS_PH;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_PH;
        }


        // ----------------------------------------------------
        // TDS
        // ----------------------------------------------------

        uint16_t tdsValue;


        if (
            ads1115.readTDS(
                tdsValue
            )
        ) {

            sensorData.tds =
                tdsValue;

            sensorData.sensorStatus |=
                SENSOR_STATUS_TDS;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_TDS;
        }
    }


    // ========================================================
    // MOTION
    // ========================================================

    void updateMotion()
    {
        float ax;
        float ay;
        float az;
        float magnitude;


        if (motion.read()) {

            ax = motion.accelerationX();
            ay = motion.accelerationY();
            az = motion.accelerationZ();
            magnitude = motion.accelerationMagnitudeMS2();

            sensorData.accelerationX =
                ax;

            sensorData.accelerationY =
                ay;

            sensorData.accelerationZ =
                az;

            sensorData.vibrationMS2 =
                magnitude;


            sensorData.sensorStatus |=
                SENSOR_STATUS_MPU6050;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_MPU6050;
        }
    }


    // ========================================================
    // GPS
    // ========================================================

    void updateGPS()
    {
        if (
            gps.hasFix()
        ) {

            sensorData.latitude =
                gps.latitude();

            sensorData.longitude =
                gps.longitude();

            sensorData.altitude =
                gps.altitude();

            sensorData.satellites =
                gps.satellites();

            sensorData.hdop =
                gps.hdop();

            sensorData.gpsFix =
                true;

            sensorData.sensorStatus |=
                SENSOR_STATUS_GPS;
        }
        else {

            sensorData.gpsFix =
                false;

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_GPS;
        }
    }


    // ========================================================
    // WEATHER
    // ========================================================

    void updateWeather()
    {
        sensorData.rainfallMM =
            weather.rainfallMMValue();


        sensorData.windSpeedMS =
            weather.windSpeedMSValue();


        if (
            weather.isInitialized()
        ) {

            sensorData.sensorStatus |=
                SENSOR_STATUS_RAINFALL;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_RAINFALL;
        }


        if (
            weather.isInitialized()
        ) {

            sensorData.sensorStatus |=
                SENSOR_STATUS_WIND_SPEED;
        }
        else {

            sensorData.sensorStatus &=
                ~SENSOR_STATUS_WIND_SPEED;
        }
    }
};


} // namespace SankatNet


#endif