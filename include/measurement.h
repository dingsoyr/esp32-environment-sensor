#pragma once

#include <stdint.h>

struct Measurement {
    uint32_t sequence;
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
    uint32_t unixTimestamp;
    bool timestampValid;
};

bool initSensor();
Measurement readMeasurement();
void printMeasurement(const Measurement& measurement);