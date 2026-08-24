#pragma once

#include <Arduino.h>

constexpr uint32_t DEFAULT_MEASUREMENT_INTERVAL_SECONDS = 3600;

struct DeviceConfig {
    String deviceName;
    uint32_t configVersion;
    uint32_t measurementIntervalSeconds;
};

DeviceConfig loadConfig();
bool saveConfig(const DeviceConfig& config);
void printConfig(const DeviceConfig& config);