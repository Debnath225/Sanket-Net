#ifndef SANKATNET_MOTION_DRIVER_H
#define SANKATNET_MOTION_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "../config/PinConfig.h"

namespace SankatNet {

class MotionDriver {
public:

    MotionDriver()
        : initialized(false),
          lastReadMillis(0),
          readCount(0),
          failureCount(0),
          ax(0.0f),
          ay(0.0f),
          az(0.0f),
          gx(0.0f),
          gy(0.0f),
          gz(0.0f),
          accelerationMagnitude(0.0f),
          vibrationMS2(0.0f) {}

    // ------------------------------------------------------------
    // Initialize MPU6050
    // ------------------------------------------------------------
    bool begin(
        TwoWire *wire = &Wire,
        uint8_t address = Pins::I2CAddress::MPU6050
    ) {

        if (!mpu.begin(address, wire)) {
            initialized = false;
            failureCount++;
            return false;
        }

        // Accelerometer: ±8 G
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

        // Gyroscope: ±500 degrees/sec
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);

        // Low-pass filtering
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        initialized = true;
        lastReadMillis = millis();

        return true;
    }

    // ------------------------------------------------------------
    // Read current motion data
    // ------------------------------------------------------------
    bool read() {

        if (!initialized) {
            return false;
        }

        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;

        if (!mpu.getEvent(&accel, &gyro, &temp)) {
            failureCount++;
            return false;
        }

        ax = accel.acceleration.x;
        ay = accel.acceleration.y;
        az = accel.acceleration.z;

        gx = gyro.gyro.x;
        gy = gyro.gyro.y;
        gz = gyro.gyro.z;

        /*
         * Total acceleration magnitude.
         *
         * When the sensor is stationary, approximately 9.81 m/s²
         * is normally observed because of gravity.
         */
        accelerationMagnitude =
            sqrtf(
                (ax * ax) +
                (ay * ay) +
                (az * az)
            );

        /*
         * Simple vibration indicator.
         *
         * Remove the gravity component so that the value represents
         * dynamic acceleration approximately.
         *
         * This is NOT an earthquake magnitude/Richter scale value.
         */
        vibrationMS2 =
            fabsf(accelerationMagnitude - GRAVITY_MS2);

        lastReadMillis = millis();
        readCount++;

        return true;
    }

    // ------------------------------------------------------------
    // Initialization status
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    // ------------------------------------------------------------
    // Accelerometer values in m/s²
    // ------------------------------------------------------------
    float accelerationX() const {
        return ax;
    }

    float accelerationY() const {
        return ay;
    }

    float accelerationZ() const {
        return az;
    }

    // ------------------------------------------------------------
    // Gyroscope values in rad/s
    // ------------------------------------------------------------
    float gyroX() const {
        return gx;
    }

    float gyroY() const {
        return gy;
    }

    float gyroZ() const {
        return gz;
    }

    // ------------------------------------------------------------
    // Total acceleration magnitude
    // ------------------------------------------------------------
    float accelerationMagnitudeMS2() const {
        return accelerationMagnitude;
    }

    // ------------------------------------------------------------
    // Vibration indicator
    // ------------------------------------------------------------
    float vibration() const {
        return vibrationMS2;
    }

    float vibrationMS2Value() const {
        return vibrationMS2;
    }

    // ------------------------------------------------------------
    // Data freshness
    // ------------------------------------------------------------
    uint32_t lastRead() const {
        return lastReadMillis;
    }

    uint32_t age() const {

        if (lastReadMillis == 0) {
            return UINT32_MAX;
        }

        return millis() - lastReadMillis;
    }

    // ------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------
    uint32_t readCountTotal() const {
        return readCount;
    }

    uint32_t failureCountTotal() const {
        return failureCount;
    }

    // ------------------------------------------------------------
    // Reset statistics
    // ------------------------------------------------------------
    void resetStatistics() {
        readCount = 0;
        failureCount = 0;
    }

private:

    static constexpr float GRAVITY_MS2 = 9.80665f;

    Adafruit_MPU6050 mpu;

    bool initialized;

    uint32_t lastReadMillis;
    uint32_t readCount;
    uint32_t failureCount;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float accelerationMagnitude;
    float vibrationMS2;
};

} // namespace SankatNet

#endif