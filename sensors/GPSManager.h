#ifndef SANKATNET_GPS_MANAGER_H
#define SANKATNET_GPS_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

#include "../config/PinConfig.h"

namespace SankatNet {

class GPSManager {
public:

    GPSManager()
        : gpsSerial(1),
          initialized(false),
          lastValidFixMillis(0),
          validFixCount(0),
          charsProcessed(0),
          lastUpdateMillis(0) {}

    // ------------------------------------------------------------
    // Initialize GPS UART
    // ------------------------------------------------------------
    bool begin(
        uint8_t rxPin = Pins::GPS_RX,
        uint8_t txPin = Pins::GPS_TX,
        uint32_t baudRate = Pins::GPS_BAUDRATE
    ) {
        gpsSerial.begin(
            baudRate,
            SERIAL_8N1,
            rxPin,
            txPin
        );

        initialized = true;
        lastUpdateMillis = millis();

        return true;
    }

    // ------------------------------------------------------------
    // Non-blocking GPS update
    //
    // Call this as frequently as possible from loop().
    // No delay() or waiting for a GPS fix is performed here.
    // ------------------------------------------------------------
    void update() {

        if (!initialized) {
            return;
        }

        while (gpsSerial.available() > 0) {

            char c = static_cast<char>(gpsSerial.read());

            charsProcessed++;

            if (tinyGps.encode(c)) {

                if (tinyGps.location.isUpdated() &&
                    tinyGps.location.isValid()) {

                    lastValidFixMillis = millis();
                    validFixCount++;
                }
            }
        }

        lastUpdateMillis = millis();
    }

    // ------------------------------------------------------------
    // GPS initialization state
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    // ------------------------------------------------------------
    // Valid and recent position
    //
    // A fix is considered usable only when:
    // 1. Latitude/longitude are valid
    // 2. At least 3 satellites are available
    // 3. The fix is not older than 5 seconds
    // ------------------------------------------------------------
    bool hasFix() const {

        if (!initialized) {
            return false;
        }

        if (!tinyGps.location.isValid()) {
            return false;
        }

        if (tinyGps.satellites.isValid() &&
            tinyGps.satellites.value() < 3) {
            return false;
        }

        if (lastValidFixMillis == 0) {
            return false;
        }

        return (millis() - lastValidFixMillis) <= MAX_FIX_AGE_MS;
    }

    // ------------------------------------------------------------
    // Latitude
    // Returns the last valid latitude even if the current fix
    // has temporarily become stale.
    // Check hasFix() before using it for current positioning.
    // ------------------------------------------------------------
    double latitude() const {

        if (!tinyGps.location.isValid()) {
            return 0.0;
        }

        return tinyGps.location.lat();
    }

    // ------------------------------------------------------------
    // Longitude
    // ------------------------------------------------------------
    double longitude() const {

        if (!tinyGps.location.isValid()) {
            return 0.0;
        }

        return tinyGps.location.lng();
    }

    // ------------------------------------------------------------
    // Altitude in meters
    // ------------------------------------------------------------
    double altitude() const {

        if (!tinyGps.altitude.isValid()) {
            return 0.0;
        }

        return tinyGps.altitude.meters();
    }

    // ------------------------------------------------------------
    // Number of satellites
    // ------------------------------------------------------------
    uint32_t satellites() const {

        if (!tinyGps.satellites.isValid()) {
            return 0;
        }

        return tinyGps.satellites.value();
    }

    // ------------------------------------------------------------
    // HDOP
    //
    // Lower HDOP generally means better positional geometry.
    // Returns infinity when unavailable.
    // ------------------------------------------------------------
    double hdop() const {

        if (!tinyGps.hdop.isValid()) {
            return NAN;
        }

        return tinyGps.hdop.hdop();
    }

    // ------------------------------------------------------------
    // Age of the latest valid GPS fix
    //
    // This is based on millis() when a valid location update was
    // received, not TinyGPSPlus's internal location.age().
    // ------------------------------------------------------------
    uint32_t fixAge() const {

        if (lastValidFixMillis == 0) {
            return UINT32_MAX;
        }

        return millis() - lastValidFixMillis;
    }

    // ------------------------------------------------------------
    // Check whether position coordinates themselves are sane
    // ------------------------------------------------------------
    bool locationIsValid() const {

        if (!tinyGps.location.isValid()) {
            return false;
        }

        const double lat = tinyGps.location.lat();
        const double lon = tinyGps.location.lng();

        if (lat < -90.0 || lat > 90.0) {
            return false;
        }

        if (lon < -180.0 || lon > 180.0) {
            return false;
        }

        return true;
    }

    // ------------------------------------------------------------
    // UTC date
    // ------------------------------------------------------------
    bool dateValid() const {
        return tinyGps.date.isValid();
    }

    uint16_t year() const {
        return tinyGps.date.isValid()
                   ? tinyGps.date.year()
                   : 0;
    }

    uint8_t month() const {
        return tinyGps.date.isValid()
                   ? tinyGps.date.month()
                   : 0;
    }

    uint8_t day() const {
        return tinyGps.date.isValid()
                   ? tinyGps.date.day()
                   : 0;
    }

    // ------------------------------------------------------------
    // UTC time
    // ------------------------------------------------------------
    bool timeValid() const {
        return tinyGps.time.isValid();
    }

    uint8_t hour() const {
        return tinyGps.time.isValid()
                   ? tinyGps.time.hour()
                   : 0;
    }

    uint8_t minute() const {
        return tinyGps.time.isValid()
                   ? tinyGps.time.minute()
                   : 0;
    }

    uint8_t second() const {
        return tinyGps.time.isValid()
                   ? tinyGps.time.second()
                   : 0;
    }

    uint16_t millisecond() const {
        return tinyGps.time.isValid()
                   ? tinyGps.time.centisecond() * 10
                   : 0;
    }

    // ------------------------------------------------------------
    // TinyGPSPlus statistics
    // ------------------------------------------------------------
    uint32_t charsProcessed() const {
        return charsProcessed;
    }

    uint32_t validFixCount() const {
        return validFixCount;
    }

    uint32_t failedChecksumCount() const {
        return tinyGps.failedChecksum();
    }

    uint32_t passedChecksumCount() const {
        return tinyGps.passedChecksum();
    }

    uint32_t lastUpdate() const {
        return lastUpdateMillis;
    }

    // ------------------------------------------------------------
    // Clear stored GPS state
    // ------------------------------------------------------------
    void reset() {

        lastValidFixMillis = 0;
        validFixCount = 0;
        charsProcessed = 0;
        lastUpdateMillis = millis();
    }

private:

    // ------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------
    static constexpr uint32_t MAX_FIX_AGE_MS = 5000;

    // ------------------------------------------------------------
    // GPS UART
    // ------------------------------------------------------------
    HardwareSerial gpsSerial;

    // ------------------------------------------------------------
    // TinyGPSPlus parser
    // ------------------------------------------------------------
    TinyGPSPlus tinyGps;

    // ------------------------------------------------------------
    // State
    // ------------------------------------------------------------
    bool initialized;

    uint32_t lastValidFixMillis;
    uint32_t validFixCount;
    uint32_t charsProcessed;
    uint32_t lastUpdateMillis;
};

} // namespace SankatNet

#endif