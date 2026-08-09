#pragma once

#include <stdint.h>

struct Measurement {
    uint32_t sequence;
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
};

bool initSensor();
Measurement readMeasurement();
void printMeasurement(const Measurement& measurement);