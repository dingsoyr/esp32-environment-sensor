#include <Preferences.h>

#include "config.h"

constexpr char SENSOR_NAMESPACE[] = "sensor";

DeviceConfig loadConfig() {
    Preferences preferences;
    preferences.begin(SENSOR_NAMESPACE, false);

    DeviceConfig config;

    config.deviceName =
        preferences.isKey("name")
            ? preferences.getString("name")
            : "unnamed";

    config.configVersion =
        preferences.isKey("version")
            ? preferences.getUInt("version")
            : 1;

    config.measurementIntervalSeconds =
        preferences.isKey("interval")
            ? preferences.getUInt("interval")
            : 30;

    preferences.end();

    return config;
}

void printConfig(const DeviceConfig& config) {
    Serial.println("=== Configuration ===");
    Serial.printf("Device name: %s\n", config.deviceName.c_str());
    Serial.printf("Config version: %lu\n", config.configVersion);
    Serial.printf(
        "Measurement interval: %lu seconds\n",
        config.measurementIntervalSeconds
    );
    Serial.println("=====================");
}