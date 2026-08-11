#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>

#include "logging.h"
#include "time_utils.h"

constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 5000;

uint32_t getCurrentUnixTimestamp() {
    return static_cast<uint32_t>(time(nullptr));
}

bool isSystemTimeValid() {
    return isUnixTimestampValid(getCurrentUnixTimestamp());
}

bool isUnixTimestampValid(uint32_t unixTimestamp) {
    return unixTimestamp >= MIN_VALID_UNIX_TIMESTAMP;
}

void setSystemTimeFromUnixTimestamp(uint32_t unixTimestamp) {
    timeval currentTime = {
        static_cast<time_t>(unixTimestamp),
        0
    };

    settimeofday(&currentTime, nullptr);
}

bool syncTimeWithNtp() {
    LOG_PRINTLN("Attempting NTP synchronization.");

    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

    const unsigned long start = millis();

    while (!isSystemTimeValid()) {
        if (millis() - start >= NTP_SYNC_TIMEOUT_MS) {
            LOG_PRINTLN("NTP synchronization timed out.");
            return false;
        }

        delay(100);
    }

    LOG_PRINTF(
        "NTP synchronization succeeded. UTC Unix time: %lu\n",
        getCurrentUnixTimestamp()
    );

    return true;
}