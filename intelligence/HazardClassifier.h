#ifndef SANKATNET_HAZARD_CLASSIFIER_H
#define SANKATNET_HAZARD_CLASSIFIER_H

#include <Arduino.h>

#include "RiskEngine.h"
#include "../network/Packet.h"

namespace SankatNet {

class HazardClassifier {
public:

    // ------------------------------------------------------------
    // Classification result
    // ------------------------------------------------------------
    struct ClassificationResult {

        HazardType hazard;

        Priority priority;

        uint8_t confidence;

        bool alertRequired;
        bool critical;

        uint8_t fireConfidence;
        uint8_t floodConfidence;
        uint8_t seismicConfidence;
        uint8_t pollutionConfidence;

        uint32_t timestamp;
    };

    HazardClassifier()
        : initialized(false),
          classificationCount(0),
          lastClassificationMillis(0) {}

    // ------------------------------------------------------------
    // Initialize classifier
    // ------------------------------------------------------------
    bool begin() {

        initialized = true;
        classificationCount = 0;
        lastClassificationMillis = 0;

        lastResult = ClassificationResult{
            HAZARD_NORMAL,
            PRIORITY_NORMAL,
            0,
            false,
            false,
            0,
            0,
            0,
            0,
            0
        };

        return true;
    }

    // ------------------------------------------------------------
    // Classify hazards from RiskEngine result
    // ------------------------------------------------------------
    ClassificationResult classify(
        const RiskEngine::RiskResult &risk
    ) {

        ClassificationResult result{};

        result.hazard = risk.hazard;
        result.priority = risk.priority;

        result.alertRequired = risk.alertRequired;
        result.critical = risk.critical;

        // --------------------------------------------------------
        // Calculate individual confidence values
        // --------------------------------------------------------
        result.fireConfidence =
            calculateConfidence(risk.fireRisk);

        result.floodConfidence =
            calculateConfidence(risk.floodRisk);

        result.seismicConfidence =
            calculateConfidence(risk.seismicRisk);

        result.pollutionConfidence =
            calculateConfidence(risk.pollutionRisk);

        // --------------------------------------------------------
        // Overall classification confidence
        // --------------------------------------------------------
        result.confidence =
            calculateOverallConfidence(risk);

        result.timestamp = millis();

        lastResult = result;

        classificationCount++;
        lastClassificationMillis = result.timestamp;

        return result;
    }

    // ------------------------------------------------------------
    // Direct classification from sensor data
    //
    // Convenience function:
    // SensorData → RiskEngine → HazardClassifier
    // ------------------------------------------------------------
    ClassificationResult classify(
        const SensorData &data,
        RiskEngine &riskEngine
    ) {

        RiskEngine::RiskResult risk =
            riskEngine.evaluate(data);

        return classify(risk);
    }

    // ------------------------------------------------------------
    // Individual risk → confidence
    //
    // This is a rule-based confidence indicator.
    // It is NOT a machine-learning probability.
    // ------------------------------------------------------------
    uint8_t calculateConfidence(
        uint8_t risk
    ) const {

        if (risk >= CRITICAL_RISK) {
            return 95;
        }

        if (risk >= HIGH_RISK) {
            return 80;
        }

        if (risk >= ALERT_RISK) {
            return 65;
        }

        if (risk >= MODERATE_RISK) {
            return 45;
        }

        if (risk >= LOW_RISK) {
            return 20;
        }

        return 0;
    }

    // ------------------------------------------------------------
    // Overall confidence
    // ------------------------------------------------------------
    uint8_t calculateOverallConfidence(
        const RiskEngine::RiskResult &risk
    ) const {

        if (risk.hazard == HAZARD_NORMAL) {
            return 0;
        }

        // --------------------------------------------------------
        // Multi-hazard classification
        // --------------------------------------------------------
        if (risk.hazard == HAZARD_MULTI) {

            uint8_t activeHazards = 0;
            uint16_t totalConfidence = 0;

            if (risk.fireRisk >= ALERT_RISK) {
                totalConfidence +=
                    calculateConfidence(risk.fireRisk);
                activeHazards++;
            }

            if (risk.floodRisk >= ALERT_RISK) {
                totalConfidence +=
                    calculateConfidence(risk.floodRisk);
                activeHazards++;
            }

            if (risk.seismicRisk >= ALERT_RISK) {
                totalConfidence +=
                    calculateConfidence(risk.seismicRisk);
                activeHazards++;
            }

            if (risk.pollutionRisk >= ALERT_RISK) {
                totalConfidence +=
                    calculateConfidence(risk.pollutionRisk);
                activeHazards++;
            }

            if (activeHazards == 0) {
                return 0;
            }

            uint8_t confidence =
                static_cast<uint8_t>(
                    totalConfidence / activeHazards
                );

            /*
             * Multi-hazard classification requires stronger
             * evidence, so cap the confidence slightly below
             * absolute certainty.
             */
            if (confidence > 95) {
                confidence = 95;
            }

            return confidence;
        }

        // --------------------------------------------------------
        // Individual hazard
        // --------------------------------------------------------
        switch (risk.hazard) {

            case HAZARD_FIRE:
                return calculateConfidence(risk.fireRisk);

            case HAZARD_FLOOD:
                return calculateConfidence(risk.floodRisk);

            case HAZARD_SEISMIC:
                return calculateConfidence(risk.seismicRisk);

            case HAZARD_POLLUTION:
                return calculateConfidence(risk.pollutionRisk);

            default:
                return 0;
        }
    }

    // ------------------------------------------------------------
    // Hazard name
    // Useful for serial debugging and gateway conversion.
    // ------------------------------------------------------------
    static const char *hazardName(
        HazardType hazard
    ) {

        switch (hazard) {

            case HAZARD_FIRE:
                return "FIRE";

            case HAZARD_FLOOD:
                return "FLOOD";

            case HAZARD_SEISMIC:
                return "SEISMIC";

            case HAZARD_POLLUTION:
                return "POLLUTION";

            case HAZARD_MULTI:
                return "MULTI_HAZARD";

            case HAZARD_NORMAL:
            default:
                return "NORMAL";
        }
    }

    // ------------------------------------------------------------
    // Priority name
    // ------------------------------------------------------------
    static const char *priorityName(
        Priority priority
    ) {

        switch (priority) {

            case PRIORITY_CRITICAL:
                return "CRITICAL";

            case PRIORITY_HIGH:
                return "HIGH";

            case PRIORITY_NORMAL:
            default:
                return "NORMAL";
        }
    }

    // ------------------------------------------------------------
    // Latest classification
    // ------------------------------------------------------------
    const ClassificationResult &last() const {
        return lastResult;
    }

    // ------------------------------------------------------------
    // Status
    // ------------------------------------------------------------
    bool isInitialized() const {
        return initialized;
    }

    uint32_t classificationCountTotal() const {
        return classificationCount;
    }

    uint32_t lastClassification() const {
        return lastClassificationMillis;
    }

private:

    // ------------------------------------------------------------
    // Rule-based thresholds
    // ------------------------------------------------------------
    static constexpr uint8_t LOW_RISK = 20;
    static constexpr uint8_t MODERATE_RISK = 40;
    static constexpr uint8_t ALERT_RISK = 60;
    static constexpr uint8_t HIGH_RISK = 80;
    static constexpr uint8_t CRITICAL_RISK = 90;

    // ------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------
    bool initialized;

    ClassificationResult lastResult;

    uint32_t classificationCount;
    uint32_t lastClassificationMillis;
};

} // namespace SankatNet

#endif