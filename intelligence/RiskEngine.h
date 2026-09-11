#ifndef SANKATNET_RISK_ENGINE_H
#define SANKATNET_RISK_ENGINE_H

#include <Arduino.h>

#include "../sensors/SensorManager.h"
#include "../network/Packet.h"

namespace SankatNet {

class RiskEngine {
public:

    // ------------------------------------------------------------
    // Risk result
    // ------------------------------------------------------------
    struct RiskResult {

        uint8_t overallRisk;

        uint8_t fireRisk;
        uint8_t floodRisk;
        uint8_t seismicRisk;
        uint8_t pollutionRisk;

        HazardType hazard;
        Priority priority;

        bool alertRequired;
        bool critical;

        uint32_t timestamp;
    };

    RiskEngine()
        : initialized(false),
          evaluationCount(0),
          lastEvaluationMillis(0) {}

    // ------------------------------------------------------------
    // Initialize risk engine
    // ------------------------------------------------------------
    bool begin() {

        initialized = true;
        evaluationCount = 0;
        lastEvaluationMillis = 0;

        lastResult = RiskResult{
            0,
            0,
            0,
            0,
            0,
            HAZARD_NORMAL,
            PRIORITY_NORMAL,
            false,
            false,
            0
        };

        return true;
    }

    // ------------------------------------------------------------
    // Evaluate all hazards
    // ------------------------------------------------------------
    RiskResult evaluate(const SensorData &data) {

        RiskResult result{};

        result.fireRisk =
            calculateFireRisk(data);

        result.floodRisk =
            calculateFloodRisk(data);

        result.seismicRisk =
            calculateSeismicRisk(data);

        result.pollutionRisk =
            calculatePollutionRisk(data);

        // --------------------------------------------------------
        // Overall risk
        // --------------------------------------------------------
        result.overallRisk =
            max(
                max(result.fireRisk, result.floodRisk),
                max(result.seismicRisk, result.pollutionRisk)
            );

        // --------------------------------------------------------
        // Determine hazard
        // --------------------------------------------------------
        result.hazard =
            determineHazard(result);

        // --------------------------------------------------------
        // Determine priority
        // --------------------------------------------------------
        result.priority =
            determinePriority(result.overallRisk);

        // --------------------------------------------------------
        // Alert condition
        // --------------------------------------------------------
        result.alertRequired =
            result.overallRisk >= ALERT_THRESHOLD;

        result.critical =
            result.overallRisk >= CRITICAL_THRESHOLD;

        result.timestamp = millis();

        lastResult = result;

        evaluationCount++;
        lastEvaluationMillis = result.timestamp;

        return result;
    }

    // ------------------------------------------------------------
    // Fire risk
    //
    // Inputs:
    // - Flame sensor
    // - Temperature
    // - MQ-2
    // - MQ-135
    // - Humidity
    // ------------------------------------------------------------
    uint8_t calculateFireRisk(
        const SensorData &data
    ) const {

        uint16_t score = 0;

        // Flame detected
        if (data.flameDetected) {
            score += 45;
        }

        // High temperature
        if (data.temperature >= FIRE_TEMP_CRITICAL) {
            score += 30;
        }
        else if (data.temperature >= FIRE_TEMP_HIGH) {
            score += 20;
        }
        else if (data.temperature >= FIRE_TEMP_WARNING) {
            score += 10;
        }

        // Combustible gas / smoke indication
        if (data.mq2 >= MQ2_CRITICAL) {
            score += 20;
        }
        else if (data.mq2 >= MQ2_HIGH) {
            score += 12;
        }
        else if (data.mq2 >= MQ2_WARNING) {
            score += 6;
        }

        // Air pollution / smoke indication
        if (data.mq135 >= MQ135_CRITICAL) {
            score += 15;
        }
        else if (data.mq135 >= MQ135_HIGH) {
            score += 10;
        }
        else if (data.mq135 >= MQ135_WARNING) {
            score += 5;
        }

        // Dry atmospheric condition
        if (data.humidity < FIRE_LOW_HUMIDITY) {
            score += 10;
        }

        return clampRisk(score);
    }

    // ------------------------------------------------------------
    // Flood risk
    //
    // Inputs:
    // - Water level
    // - Rainfall
    // - Soil moisture
    // ------------------------------------------------------------
    uint8_t calculateFloodRisk(
        const SensorData &data
    ) const {

        uint16_t score = 0;

        // Water level
        if (data.waterLevel >= WATER_CRITICAL) {
            score += 50;
        }
        else if (data.waterLevel >= WATER_HIGH) {
            score += 35;
        }
        else if (data.waterLevel >= WATER_WARNING) {
            score += 20;
        }

        // Rainfall
        if (data.rainfallMM >= RAINFALL_CRITICAL) {
            score += 35;
        }
        else if (data.rainfallMM >= RAINFALL_HIGH) {
            score += 25;
        }
        else if (data.rainfallMM >= RAINFALL_WARNING) {
            score += 12;
        }

        // Saturated soil
        if (data.soilMoisture >= SOIL_CRITICAL) {
            score += 20;
        }
        else if (data.soilMoisture >= SOIL_HIGH) {
            score += 12;
        }
        else if (data.soilMoisture >= SOIL_WARNING) {
            score += 5;
        }

        return clampRisk(score);
    }

    // ------------------------------------------------------------
    // Seismic / vibration risk
    //
    // IMPORTANT:
    // This is a local vibration indicator.
    // It is NOT an earthquake magnitude estimator.
    // ------------------------------------------------------------
    uint8_t calculateSeismicRisk(
        const SensorData &data
    ) const {

        uint16_t score = 0;

        if (data.vibrationMS2 >= VIBRATION_CRITICAL) {
            score += 70;
        }
        else if (data.vibrationMS2 >= VIBRATION_HIGH) {
            score += 50;
        }
        else if (data.vibrationMS2 >= VIBRATION_WARNING) {
            score += 25;
        }

        // Additional acceleration consistency indicator
        const float accelerationMagnitude = sqrtf(
            (data.accelerationX * data.accelerationX) +
            (data.accelerationY * data.accelerationY) +
            (data.accelerationZ * data.accelerationZ)
        );

        if (accelerationMagnitude >= ACCELERATION_HIGH) {
            score += 20;
        }

        return clampRisk(score);
    }

    // ------------------------------------------------------------
    // Pollution risk
    //
    // Inputs:
    // - MQ-2
    // - MQ-135
    // - Turbidity
    // ------------------------------------------------------------
    uint8_t calculatePollutionRisk(
        const SensorData &data
    ) const {

        uint16_t score = 0;

        // MQ-2
        if (data.mq2 >= MQ2_CRITICAL) {
            score += 35;
        }
        else if (data.mq2 >= MQ2_HIGH) {
            score += 25;
        }
        else if (data.mq2 >= MQ2_WARNING) {
            score += 12;
        }

        // MQ-135
        if (data.mq135 >= MQ135_CRITICAL) {
            score += 40;
        }
        else if (data.mq135 >= MQ135_HIGH) {
            score += 30;
        }
        else if (data.mq135 >= MQ135_WARNING) {
            score += 15;
        }

        // Water turbidity
        if (data.turbidity >= TURBIDITY_CRITICAL) {
            score += 25;
        }
        else if (data.turbidity >= TURBIDITY_HIGH) {
            score += 15;
        }
        else if (data.turbidity >= TURBIDITY_WARNING) {
            score += 8;
        }

        return clampRisk(score);
    }

    // ------------------------------------------------------------
    // Determine dominant hazard
    // ------------------------------------------------------------
    HazardType determineHazard(
        const RiskResult &result
    ) const {

        uint8_t hazardCount = 0;

        if (result.fireRisk >= ALERT_THRESHOLD) {
            hazardCount++;
        }

        if (result.floodRisk >= ALERT_THRESHOLD) {
            hazardCount++;
        }

        if (result.seismicRisk >= ALERT_THRESHOLD) {
            hazardCount++;
        }

        if (result.pollutionRisk >= ALERT_THRESHOLD) {
            hazardCount++;
        }

        // Multiple simultaneous hazards
        if (hazardCount >= 2) {
            return HAZARD_MULTI;
        }

        // Find dominant individual hazard
        uint8_t maximum = 0;
        HazardType dominant = HAZARD_NORMAL;

        if (result.fireRisk > maximum) {
            maximum = result.fireRisk;
            dominant = HAZARD_FIRE;
        }

        if (result.floodRisk > maximum) {
            maximum = result.floodRisk;
            dominant = HAZARD_FLOOD;
        }

        if (result.seismicRisk > maximum) {
            maximum = result.seismicRisk;
            dominant = HAZARD_SEISMIC;
        }

        if (result.pollutionRisk > maximum) {
            maximum = result.pollutionRisk;
            dominant = HAZARD_POLLUTION;
        }

        return dominant;
    }

    // ------------------------------------------------------------
    // Determine packet priority
    // ------------------------------------------------------------
    Priority determinePriority(
        uint8_t risk
    ) const {

        if (risk >= CRITICAL_THRESHOLD) {
            return PRIORITY_CRITICAL;
        }

        if (risk >= ALERT_THRESHOLD) {
            return PRIORITY_HIGH;
        }

        return PRIORITY_NORMAL;
    }

    // ------------------------------------------------------------
    // Latest result
    // ------------------------------------------------------------
    const RiskResult &last() const {
        return lastResult;
    }

    // ------------------------------------------------------------
    // Status
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    uint32_t evaluationCountTotal() const {
        return evaluationCount;
    }

    uint32_t lastEvaluation() const {
        return lastEvaluationMillis;
    }

private:

    // ============================================================
    // Risk thresholds
    //
    // These are engineering placeholders for the prototype.
    // They MUST be calibrated against real sensor behavior and
    // site-specific environmental conditions before deployment.
    // ============================================================

    static constexpr uint8_t ALERT_THRESHOLD = 60;
    static constexpr uint8_t CRITICAL_THRESHOLD = 80;

    // ------------------------------------------------------------
    // Fire thresholds
    // ------------------------------------------------------------
    static constexpr float FIRE_TEMP_WARNING = 40.0f;
    static constexpr float FIRE_TEMP_HIGH = 50.0f;
    static constexpr float FIRE_TEMP_CRITICAL = 60.0f;

    static constexpr uint16_t MQ2_WARNING = 1500;
    static constexpr uint16_t MQ2_HIGH = 2500;
    static constexpr uint16_t MQ2_CRITICAL = 3500;

    static constexpr uint16_t MQ135_WARNING = 1500;
    static constexpr uint16_t MQ135_HIGH = 2500;
    static constexpr uint16_t MQ135_CRITICAL = 3500;

    static constexpr float FIRE_LOW_HUMIDITY = 30.0f;

    // ------------------------------------------------------------
    // Flood thresholds
    // ------------------------------------------------------------
    static constexpr uint16_t WATER_WARNING = 500;
    static constexpr uint16_t WATER_HIGH = 700;
    static constexpr uint16_t WATER_CRITICAL = 850;

    static constexpr float RAINFALL_WARNING = 10.0f;
    static constexpr float RAINFALL_HIGH = 30.0f;
    static constexpr float RAINFALL_CRITICAL = 60.0f;

    static constexpr float SOIL_WARNING = 70.0f;
    static constexpr float SOIL_HIGH = 85.0f;
    static constexpr float SOIL_CRITICAL = 95.0f;

    // ------------------------------------------------------------
    // Seismic / vibration thresholds
    // ------------------------------------------------------------
    static constexpr float VIBRATION_WARNING = 1.0f;
    static constexpr float VIBRATION_HIGH = 2.5f;
    static constexpr float VIBRATION_CRITICAL = 5.0f;

    static constexpr float ACCELERATION_HIGH = 12.0f;

    // ------------------------------------------------------------
    // Pollution thresholds
    // ------------------------------------------------------------
    static constexpr uint16_t TURBIDITY_WARNING = 400;
    static constexpr uint16_t TURBIDITY_HIGH = 700;
    static constexpr uint16_t TURBIDITY_CRITICAL = 1000;

    // ------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------
    bool initialized;

    RiskResult lastResult;

    uint32_t evaluationCount;
    uint32_t lastEvaluationMillis;

    // ------------------------------------------------------------
    // Clamp risk to 0–100
    // ------------------------------------------------------------
    static uint8_t clampRisk(
        uint16_t value
    ) {

        if (value > 100) {
            return 100;
        }

        return static_cast<uint8_t>(value);
    }
};

} // namespace SankatNet

#endif