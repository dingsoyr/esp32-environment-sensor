#include <Preferences.h>

#include "config.h"
#include "logging.h"

constexpr char SENSOR_NAMESPACE[] = "sensor";

DeviceConfig loadConfig() {
    Preferences preferences;
    preferences.begin(SENSOR_NAMESPACE, false);

    // Change interval for testing purposes
    // preferences.putUInt("interval", 10);

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
    LOG_PRINTLN("=== Configuration ===");
    LOG_PRINTF("Device name: %s\n", config.deviceName.c_str());
    LOG_PRINTF("Config version: %lu\n", config.configVersion);
    LOG_PRINTF(
        "Measurement interval: %lu seconds\n",
        config.measurementIntervalSeconds
    );
    LOG_PRINTLN("=====================");
}