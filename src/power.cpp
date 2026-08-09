#include <Arduino.h>
#include <esp_sleep.h>

#include "power.h"

void goToSleep(uint32_t sleepDurationSeconds) {
    Serial.printf(
        "Going to deep sleep for %lu seconds...\n",
        sleepDurationSeconds
    );

    Serial.flush();

    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(sleepDurationSeconds) * 1000000ULL
    );

    esp_deep_sleep_start();
}