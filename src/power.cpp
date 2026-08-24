#include <Arduino.h>
#include <esp_sleep.h>

#include "logging.h"
#include "power.h"

#ifndef DISABLE_DEEP_SLEEP
#define DISABLE_DEEP_SLEEP 0
#endif

void goToSleep(uint32_t sleepDurationSeconds) {
#if DISABLE_DEEP_SLEEP
    LOG_PRINTF(
        "Deep sleep disabled for debug build. Staying awake instead of sleeping for %lu seconds.\n",
        sleepDurationSeconds
    );

    LOG_FLUSH();

    while (true) {
        delay(1000);
    }
#else
    LOG_PRINTF(
        "Going to deep sleep for %lu seconds...\n",
        sleepDurationSeconds
    );

    LOG_FLUSH();

    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(sleepDurationSeconds) * 1000000ULL
    );

    esp_deep_sleep_start();
#endif
}
