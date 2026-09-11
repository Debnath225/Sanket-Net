#ifndef SANKATNET_GATEWAY_WATCHDOG_H
#define SANKATNET_GATEWAY_WATCHDOG_H

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "../config/GatewayConfig.h"


class GatewayWatchdog
{
public:

    GatewayWatchdog()
        : _initialized(false)
    {
    }


    /*
     * ========================================================
     * BEGIN
     * ========================================================
     */

    void begin()
    {
        if (_initialized)
        {
            return;
        }


        /*
         * Configure ESP32 Task Watchdog.
         *
         * Timeout is intentionally longer than the
         * normal gateway processing cycle.
         */

        esp_task_wdt_config_t config =
        {
            .timeout_ms = 10000,
            .idle_core_mask = 0,
            .trigger_panic = true
        };


        esp_task_wdt_init(&config);


        /*
         * Register the current Arduino loop task.
         */

        esp_task_wdt_add(NULL);


        _initialized = true;
    }


    /*
     * ========================================================
     * FEED
     * ========================================================
     */

    void feed()
    {
        if (!_initialized)
        {
            return;
        }

        esp_task_wdt_reset();
    }


    /*
     * ========================================================
     * END
     * ========================================================
     */

    void end()
    {
        if (!_initialized)
        {
            return;
        }


        esp_task_wdt_delete(NULL);

        _initialized = false;
    }


    /*
     * ========================================================
     * STATUS
     * ========================================================
     */

    bool isInitialized() const
    {
        return _initialized;
    }


private:

    bool _initialized;
};

using Watchdog = GatewayWatchdog;


#endif