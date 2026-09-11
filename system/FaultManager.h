#ifndef SANKATNET_FAULT_MANAGER_H
#define SANKATNET_FAULT_MANAGER_H

#include <Arduino.h>

#include "../config/NodeConfig.h"


namespace SankatNet {


class FaultManager {

public:

    // ========================================================
    // FAULT TYPES
    // ========================================================

    enum FaultType : uint32_t {

        FAULT_NONE =
            0,

        FAULT_BME280 =
            (1UL << 0),

        FAULT_ADS1115 =
            (1UL << 1),

        FAULT_GPS =
            (1UL << 2),

        FAULT_MPU6050 =
            (1UL << 3),

        FAULT_WEATHER =
            (1UL << 4),

        FAULT_LORA =
            (1UL << 5),

        FAULT_COMMUNICATION =
            (1UL << 6),

        FAULT_LOW_BATTERY =
            (1UL << 7),

        FAULT_CRITICAL_BATTERY =
            (1UL << 8),

        FAULT_SENSOR_DATA =
            (1UL << 9),

        FAULT_PACKET_QUEUE =
            (1UL << 10),

        FAULT_WATCHDOG =
            (1UL << 11),

        FAULT_CONFIGURATION =
            (1UL << 12),

        FAULT_SECURITY =
            (1UL << 13),

        WATCHDOG = FAULT_WATCHDOG,
        CONFIGURATION = FAULT_CONFIGURATION,
        SENSOR_DATA = FAULT_SENSOR_DATA,
        LOW_BATTERY = FAULT_LOW_BATTERY,
        LORA = FAULT_LORA,
        SECURITY = FAULT_SECURITY,
        COMMUNICATION = FAULT_COMMUNICATION,
        PACKET_QUEUE = FAULT_PACKET_QUEUE
    };


    // ========================================================
    // INITIALIZATION
    // ========================================================

    void begin()
    {
        activeFaults =
            FAULT_NONE;

        latchedFaults =
            FAULT_NONE;

        totalFaultEvents =
            0;

        sensorFailureCount =
            0;

        communicationFailureCount =
            0;

        initialized =
            true;
    }


    // ========================================================
    // RAISE FAULT
    // ========================================================

    void raise(
        FaultType fault
    )
    {
        if (
            fault == FAULT_NONE
        ) {
            return;
        }


        uint32_t mask =
            static_cast<uint32_t>(
                fault
            );


        /*
         * Record only a new transition as a new event.
         */

        if (
            (activeFaults & mask) == 0
        ) {

            totalFaultEvents++;
        }


        activeFaults |=
            mask;

        latchedFaults |=
            mask;
    }


    // ========================================================
    // CLEAR FAULT
    // ========================================================

    void clear(
        FaultType fault
    )
    {
        if (
            fault == FAULT_NONE
        ) {
            return;
        }


        uint32_t mask =
            static_cast<uint32_t>(
                fault
            );


        activeFaults &=
            ~mask;
    }


    // ========================================================
    // CHECK FAULT
    // ========================================================

    bool hasFault(
        FaultType fault
    ) const
    {
        uint32_t mask =
            static_cast<uint32_t>(
                fault
            );


        return (
            activeFaults &
            mask
        ) != 0;
    }


    bool hasAnyFault() const
    {
        return (
            activeFaults !=
            FAULT_NONE
        );
    }


    // ========================================================
    // GET ACTIVE FAULTS
    // ========================================================

    uint32_t getActiveFaults() const
    {
        return activeFaults;
    }


    uint32_t getLatchedFaults() const
    {
        return latchedFaults;
    }


    // ========================================================
    // SENSOR FAILURE TRACKING
    // ========================================================

    void sensorFailure(
        FaultType sensorFault
    )
    {
        sensorFailureCount++;

        raise(
            sensorFault
        );
    }


    void sensorRecovered(
        FaultType sensorFault
    )
    {
        clear(
            sensorFault
        );
    }


    uint8_t getSensorFailureCount() const
    {
        return sensorFailureCount;
    }


    // ========================================================
    // COMMUNICATION FAILURE
    // ========================================================

    void communicationFailure()
    {
        if (
            communicationFailureCount <
            255
        ) {

            communicationFailureCount++;
        }


        raise(
            FAULT_COMMUNICATION
        );
    }


    void communicationRecovered()
    {
        communicationFailureCount =
            0;

        clear(
            FAULT_COMMUNICATION
        );
    }


    uint8_t getCommunicationFailureCount() const
    {
        return communicationFailureCount;
    }


    // ========================================================
    // BATTERY MONITORING
    // ========================================================

    void updateBattery(
        uint16_t batteryMV
    )
    {
        lastBatteryMV =
            batteryMV;


        if (
            batteryMV <=
            BatteryConfig::CRITICAL_BATTERY_MV
        ) {

            raise(
                FAULT_CRITICAL_BATTERY
            );

            /*
             * Critical battery takes precedence over the
             * normal low-battery state.
             */

            clear(
                FAULT_LOW_BATTERY
            );

            return;
        }


        if (
            batteryMV <=
            BatteryConfig::LOW_BATTERY_MV
        ) {

            raise(
                FAULT_LOW_BATTERY
            );

            clear(
                FAULT_CRITICAL_BATTERY
            );

            return;
        }


        /*
         * Battery is healthy.
         */

        clear(
            FAULT_LOW_BATTERY
        );

        clear(
            FAULT_CRITICAL_BATTERY
        );
    }


    uint16_t getBatteryMV() const
    {
        return lastBatteryMV;
    }


    bool isBatteryCritical() const
    {
        return hasFault(
            FAULT_CRITICAL_BATTERY
        );
    }


    bool isBatteryLow() const
    {
        return hasFault(
            FAULT_LOW_BATTERY
        );
    }


    // ========================================================
    // FAULT SEVERITY
    // ========================================================

    enum class Severity : uint8_t {

        NORMAL = 0,

        WARNING = 1,

        CRITICAL = 2
    };


    Severity getSeverity() const
    {
        if (
            hasFault(
                FAULT_CRITICAL_BATTERY
            ) ||
            hasFault(
                FAULT_WATCHDOG
            ) ||
            hasFault(
                FAULT_SECURITY
            )
        ) {

            return Severity::CRITICAL;
        }


        if (
            hasAnyFault()
        ) {

            return Severity::WARNING;
        }


        return Severity::NORMAL;
    }


    // ========================================================
    // SYSTEM HEALTH
    // ========================================================

    bool isHealthy() const
    {
        return (
            activeFaults ==
            FAULT_NONE
        );
    }


    /*
     * A node can still be operational even when one sensor
     * fails. This is different from isHealthy().
     */

    bool isOperational() const
    {
        uint32_t nonOperationalFaults =
            static_cast<uint32_t>(
                FAULT_LORA |
                FAULT_SECURITY |
                FAULT_WATCHDOG |
                FAULT_CRITICAL_BATTERY
            );


        return (
            (activeFaults &
             nonOperationalFaults) == 0
        );
    }


    // ========================================================
    // FAULT THRESHOLDS
    // ========================================================

    bool sensorFailureLimitExceeded() const
    {
        return (
            sensorFailureCount >=
            SystemLimits::MAX_SENSOR_FAILURES
        );
    }


    bool communicationLimitExceeded() const
    {
        return (
            communicationFailureCount >=
            SystemLimits::MAX_COMM_FAILURES
        );
    }


    // ========================================================
    // STATISTICS
    // ========================================================

    uint32_t getTotalFaultEvents() const
    {
        return totalFaultEvents;
    }


    // ========================================================
    // CLEAR ALL ACTIVE FAULTS
    // ========================================================

    void clearActiveFaults()
    {
        activeFaults =
            FAULT_NONE;

        sensorFailureCount =
            0;

        communicationFailureCount =
            0;
    }


    // ========================================================
    // CLEAR COMPLETE HISTORY
    // ========================================================

    void reset()
    {
        activeFaults =
            FAULT_NONE;

        latchedFaults =
            FAULT_NONE;

        totalFaultEvents =
            0;

        sensorFailureCount =
            0;

        communicationFailureCount =
            0;

        lastBatteryMV =
            0;
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
    // STATE
    // ========================================================

    bool initialized =
        false;


    uint32_t activeFaults =
        FAULT_NONE;


    /*
     * Latched faults remain recorded even after recovery.
     * Useful for diagnostics.
     */

    uint32_t latchedFaults =
        FAULT_NONE;


    // ========================================================
    // COUNTERS
    // ========================================================

    uint32_t totalFaultEvents =
        0;


    uint8_t sensorFailureCount =
        0;


    uint8_t communicationFailureCount =
        0;


    // ========================================================
    // BATTERY
    // ========================================================

    uint16_t lastBatteryMV =
        0;
};


using FaultType = FaultManager::FaultType;


} // namespace SankatNet


#endif