#ifndef SANKATNET_WATCHDOG_H
#define SANKATNET_WATCHDOG_H

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "../config/NodeConfig.h"


namespace SankatNet {


class Watchdog {

public:

    // ========================================================
    // CONFIGURATION
    // ========================================================

    /*
     * Watchdog timeout.
     *
     * If the main application stops feeding the watchdog
     * for this period, ESP32 will reset the affected task/
     * system according to the configured ESP-IDF behaviour.
     */

    static constexpr uint32_t TIMEOUT_SECONDS =
        10;


    // ========================================================
    // INITIALIZATION
    // ========================================================

    bool begin()
    {
        if (
            initialized
        ) {
            return true;
        }


        /*
         * Configure ESP32 Task Watchdog.
         *
         * The exact API differs slightly between ESP32 core /
         * ESP-IDF versions, so this implementation uses the
         * current Arduino-ESP32 Task WDT interface.
         */

        esp_task_wdt_config_t config = {
            .timeout_ms =
                TIMEOUT_SECONDS * 1000,

            .idle_core_mask =
                (1 << portNUM_PROCESSORS) - 1,

            .trigger_panic =
                true
        };


        esp_err_t result =
            esp_task_wdt_init(
                &config
            );


        /*
         * ESP_ERR_INVALID_STATE means the watchdog was already
         * initialized by another component.
         *
         * That is not necessarily an error for our application.
         */

        if (
            result != ESP_OK &&
            result != ESP_ERR_INVALID_STATE
        ) {

            initialized =
                false;

            return false;
        }


        /*
         * Subscribe the current Arduino loop task.
         */

        result =
            esp_task_wdt_add(
                nullptr
            );


        if (
            result != ESP_OK &&
            result != ESP_ERR_INVALID_STATE
        ) {

            initialized =
                false;

            return false;
        }


        lastFeedTime =
            millis();


        initialized =
            true;


        return true;
    }


    // ========================================================
    // FEED
    // ========================================================

    void feed()
    {
        if (
            !initialized
        ) {
            return;
        }


        esp_task_wdt_reset();

        lastFeedTime =
            millis();

        feedCount++;
    }


    // ========================================================
    // PERIODIC UPDATE
    // ========================================================

    void update()
    {
        /*
         * The main loop should call update() frequently.
         */

        feed();
    }


    // ========================================================
    // MANUAL PAUSE
    // ========================================================

    bool pause()
    {
        if (
            !initialized
        ) {
            return false;
        }


        esp_err_t result =
            esp_task_wdt_delete(
                nullptr
            );


        if (
            result == ESP_OK ||
            result == ESP_ERR_INVALID_STATE
        ) {

            paused =
                true;

            return true;
        }


        return false;
    }


    // ========================================================
    // RESUME
    // ========================================================

    bool resume()
    {
        if (
            !initialized
        ) {
            return false;
        }


        if (
            !paused
        ) {
            return true;
        }


        esp_err_t result =
            esp_task_wdt_add(
                nullptr
            );


        if (
            result == ESP_OK ||
            result == ESP_ERR_INVALID_STATE
        ) {

            paused =
                false;

            lastFeedTime =
                millis();

            return true;
        }


        return false;
    }


    // ========================================================
    // STATUS
    // ========================================================

    bool isInitialized() const
    {
        return initialized;
    }


    bool isPaused() const
    {
        return paused;
    }


    uint32_t getFeedCount() const
    {
        return feedCount;
    }


    uint32_t getLastFeedTime() const
    {
        return lastFeedTime;
    }


private:

    bool initialized =
        false;

    bool paused =
        false;

    uint32_t lastFeedTime =
        0;

    uint32_t feedCount =
        0;
};


} // namespace SankatNet


#endif