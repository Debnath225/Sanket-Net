#ifndef SANKATNET_CALIBRATION_H
#define SANKATNET_CALIBRATION_H

#include <Arduino.h>
#include <math.h>

/*
 * ============================================================
 * Sankat-Net : Sensor Calibration
 * ============================================================
 *
 * IMPORTANT:
 * These are DEFAULT calibration parameters.
 *
 * Real sensor calibration must be performed using the actual
 * hardware, sensor module, supply voltage and environment.
 *
 * Do not treat MQ-series raw ADC values as ppm until a proper
 * calibration procedure has been performed.
 *
 * ============================================================
 */

namespace SankatNet {
namespace Calibration {


// ============================================================
// 1. BME280
// ============================================================
//
// BME280 library normally performs the sensor's internal
// factory calibration automatically.
//
// Therefore no manual conversion is required here.
//

namespace BME280 {

constexpr float TEMPERATURE_OFFSET_C = 0.0f;
constexpr float HUMIDITY_OFFSET_PERCENT = 0.0f;
constexpr float PRESSURE_OFFSET_PA = 0.0f;

inline float calibrateTemperature(float value)
{
    return value + TEMPERATURE_OFFSET_C;
}

inline float calibrateHumidity(float value)
{
    return constrain(
        value + HUMIDITY_OFFSET_PERCENT,
        0.0f,
        100.0f
    );
}

inline float calibratePressure(float value)
{
    return value + PRESSURE_OFFSET_PA;
}

}


// ============================================================
// 2. MQ-2
// ============================================================
//
// MQ-2 is an analog gas/smoke sensor.
//
// IMPORTANT:
// Raw ADC value is NOT automatically equivalent to ppm.
//
// For the first prototype we retain a normalized ADC value.
// Actual gas concentration estimation requires calibration
// using the target gas and sensor Rs/R0 relationship.
//

namespace MQ2 {

constexpr uint16_t ADC_MIN = 0;
constexpr uint16_t ADC_MAX = 4095;

/*
 * Offset and gain allow later field calibration.
 */
constexpr float OFFSET = 0.0f;
constexpr float GAIN = 1.0f;

inline uint16_t calibrate(uint16_t raw)
{
    float value =
        (static_cast<float>(raw) * GAIN) + OFFSET;

    value = constrain(
        value,
        static_cast<float>(ADC_MIN),
        static_cast<float>(ADC_MAX)
    );

    return static_cast<uint16_t>(value);
}

}


// ============================================================
// 3. MQ-135
// ============================================================
//
// Like MQ-2, the raw ADC reading is retained unless a proper
// calibration process has established an R0 and gas-response
// model.
//

namespace MQ135 {

constexpr uint16_t ADC_MIN = 0;
constexpr uint16_t ADC_MAX = 4095;

constexpr float OFFSET = 0.0f;
constexpr float GAIN = 1.0f;

inline uint16_t calibrate(uint16_t raw)
{
    float value =
        (static_cast<float>(raw) * GAIN) + OFFSET;

    value = constrain(
        value,
        static_cast<float>(ADC_MIN),
        static_cast<float>(ADC_MAX)
    );

    return static_cast<uint16_t>(value);
}

}


// ============================================================
// 4. WATER LEVEL
// ============================================================
//
// Default representation is a normalized 0-1000 value.
//
// The actual conversion to physical water height depends on
// the selected water-level sensor.
//

namespace WaterLevel {

constexpr uint16_t RAW_MIN = 0;
constexpr uint16_t RAW_MAX = 4095;

constexpr uint16_t LEVEL_MIN = 0;
constexpr uint16_t LEVEL_MAX = 1000;

inline uint16_t normalize(uint16_t raw)
{
    raw = constrain(raw, RAW_MIN, RAW_MAX);

    return static_cast<uint16_t>(
        map(
            raw,
            RAW_MIN,
            RAW_MAX,
            LEVEL_MIN,
            LEVEL_MAX
        )
    );
}

}


// ============================================================
// 5. SOIL MOISTURE
// ============================================================
//
// Capacitive soil sensors generally have different dry/wet
// endpoints. These values MUST be adjusted after measuring
// the actual sensor.
//

namespace SoilMoisture {

/*
 * Example ADC endpoints.
 *
 * IMPORTANT:
 * Measure your actual sensor before deployment.
 */
constexpr uint16_t DRY_ADC = 3200;
constexpr uint16_t WET_ADC = 1400;

inline uint8_t percentage(uint16_t raw)
{
    /*
     * Higher ADC value usually means drier soil for many
     * capacitive modules.
     */

    if (raw >= DRY_ADC) {
        return 0;
    }

    if (raw <= WET_ADC) {
        return 100;
    }

    long value = map(
        raw,
        WET_ADC,
        DRY_ADC,
        100,
        0
    );

    return static_cast<uint8_t>(
        constrain(value, 0L, 100L)
    );
}

}


// ============================================================
// 6. SOIL TEMPERATURE — DS18B20
// ============================================================
//
// DS18B20 already provides temperature in degrees Celsius.
// Only a small field-calibration offset is provided.
//

namespace SoilTemperature {

constexpr float OFFSET_C = 0.0f;

inline float calibrate(float value)
{
    return value + OFFSET_C;
}

}


// ============================================================
// 7. TURBIDITY
// ============================================================
//
// Turbidity modules normally provide an analog voltage.
// Converting directly to NTU requires calibration against
// known turbidity standards.
//
// Until that calibration is available, retain a normalized
// sensor value.
//

namespace Turbidity {

constexpr uint16_t ADC_MIN = 0;
constexpr uint16_t ADC_MAX = 32767;

constexpr float OFFSET = 0.0f;
constexpr float GAIN = 1.0f;

inline uint16_t calibrate(uint16_t raw)
{
    float value =
        (static_cast<float>(raw) * GAIN) + OFFSET;

    value = constrain(
        value,
        static_cast<float>(ADC_MIN),
        static_cast<float>(ADC_MAX)
    );

    return static_cast<uint16_t>(value);
}

}


// ============================================================
// 8. pH SENSOR
// ============================================================
//
// The pH probe MUST use a suitable pH interface circuit.
//
// pH conversion is normally based on the calibrated voltage
// from the interface board.
//
// Default:
//     pH = SLOPE * voltage + OFFSET
//
// These values are intentionally configurable.
//

namespace PH {

constexpr float SLOPE = -5.70f;
constexpr float OFFSET = 21.34f;

constexpr float MIN_PH = 0.0f;
constexpr float MAX_PH = 14.0f;

inline float fromVoltage(float voltage)
{
    float value =
        (SLOPE * voltage) + OFFSET;

    return constrain(
        value,
        MIN_PH,
        MAX_PH
    );
}

}


// ============================================================
// 9. TDS
// ============================================================
//
// TDS conversion depends on the particular TDS module,
// temperature compensation and calibration solution.
//
// This implementation provides a basic configurable model.
// It should NOT be considered laboratory-grade TDS measurement.
//

namespace TDS {

constexpr float CALIBRATION_FACTOR = 1.0f;
constexpr float OFFSET_PPM = 0.0f;

inline float compensateTemperature(
    float tds,
    float temperatureC
)
{
    /*
     * Approximate temperature compensation.
     *
     * Reference temperature = 25 °C
     */
    constexpr float REFERENCE_TEMP = 25.0f;
    constexpr float TEMP_COEFFICIENT = 0.02f;

    float compensation =
        1.0f +
        TEMP_COEFFICIENT *
        (temperatureC - REFERENCE_TEMP);

    if (compensation <= 0.0f) {
        compensation = 1.0f;
    }

    return tds / compensation;
}

inline uint16_t calibrate(
    float rawTds,
    float temperatureC
)
{
    float value =
        rawTds * CALIBRATION_FACTOR;

    value =
        compensateTemperature(value, temperatureC);

    value += OFFSET_PPM;

    value = constrain(value, 0.0f, 65535.0f);

    return static_cast<uint16_t>(value);
}

}


// ============================================================
// 10. RAIN GAUGE
// ============================================================
//
// Rain gauges normally generate one pulse per fixed amount
// of collected rainfall.
//
// Set the actual value according to the physical rain gauge.
//

namespace RainGauge {

/*
 * Example:
 * 1 pulse = 0.2794 mm
 *
 * Replace with the actual specification of your gauge.
 */
constexpr float MM_PER_PULSE = 0.2794f;

inline float rainfallFromPulses(uint32_t pulses)
{
    return
        static_cast<float>(pulses) *
        MM_PER_PULSE;
}

}


// ============================================================
// 11. ANEMOMETER
// ============================================================
//
// Conversion depends on the manufacturer's pulse
// specification.
//
// Keep the calibration factor configurable.
//

namespace Anemometer {

/*
 * Default configurable factor.
 *
 * Replace according to the selected anemometer.
 */
constexpr float WIND_SPEED_PER_PULSE_HZ = 1.0f;

inline float windSpeedFromFrequency(float frequencyHz)
{
    if (frequencyHz < 0.0f) {
        return 0.0f;
    }

    return frequencyHz *
           WIND_SPEED_PER_PULSE_HZ;
}

}


// ============================================================
// 12. BATTERY
// ============================================================
//
// The ESP32 ADC does not directly measure a high battery
// voltage. A resistor divider must scale the battery voltage
// into the ADC-safe range.
//
// Configure these values according to the actual resistor
// divider used in hardware.
//

namespace Battery {

constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;

/*
 * Default 100k / 100k divider:
 *
 * Vadc = Vbattery * 100k / (100k + 100k)
 *
 * Therefore:
 *
 * Vbattery = Vadc * 2
 */
constexpr float R_TOP = 100000.0f;
constexpr float R_BOTTOM = 100000.0f;

constexpr float DIVIDER_RATIO =
    (R_TOP + R_BOTTOM) / R_BOTTOM;

inline float voltageFromAdc(
    uint16_t adcValue,
    uint16_t adcMax = 4095
)
{
    if (adcMax == 0) {
        return 0.0f;
    }

    float adcVoltage =
        (static_cast<float>(adcValue) /
         static_cast<float>(adcMax)) *
        ADC_REFERENCE_VOLTAGE;

    return adcVoltage * DIVIDER_RATIO;
}

inline uint16_t millivoltsFromAdc(
    uint16_t adcValue,
    uint16_t adcMax = 4095
)
{
    float voltage =
        voltageFromAdc(adcValue, adcMax);

    voltage = constrain(voltage, 0.0f, 6.6f);

    return static_cast<uint16_t>(
        voltage * 1000.0f
    );
}

}


// ============================================================
// 13. GENERIC ADC NORMALIZATION
// ============================================================

namespace ADC {

inline uint16_t normalize12Bit(
    uint16_t raw,
    uint16_t outputMax = 1000
)
{
    raw = constrain(raw, 0U, 4095U);

    return static_cast<uint16_t>(
        (static_cast<uint32_t>(raw) * outputMax) /
        4095UL
    );
}

}


// ============================================================
// 14. CALIBRATION VERSION
// ============================================================
//
// Increment this whenever field calibration constants are
// changed. The value can later be included in diagnostics or
// configuration packets.
//

constexpr uint16_t CALIBRATION_VERSION = 1;


} // namespace Calibration
} // namespace SankatNet

#endif