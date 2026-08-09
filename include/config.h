#pragma once

#include <Arduino.h>

struct DeviceConfig {
    String deviceName;
    uint32_t configVersion;
    uint32_t measurementIntervalSeconds;
};

DeviceConfig loadConfig();
void printConfig(const DeviceConfig& config);