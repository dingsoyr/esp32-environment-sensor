#pragma once

#include <type_traits>
#include <stdint.h>

struct Measurement {
    uint32_t sequence;
    uint32_t unixTimestamp;
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
    float batteryVoltage;
    uint8_t batteryPercent;
    bool timestampValid;
};

static_assert(
    std::is_trivially_copyable<Measurement>::value,
    "Measurement must remain trivially copyable for NVS storage."
);

static_assert(
    sizeof(Measurement) == 28,
    "Measurement layout changed. Update the buffered measurement format version deliberately."
);

bool initSensor();
Measurement readMeasurement();
void printMeasurement(const Measurement& measurement);
