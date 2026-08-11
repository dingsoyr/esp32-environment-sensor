#include <Preferences.h>

#include "config.h"
#include "logging.h"

constexpr char SENSOR_NAMESPACE[] = "sensor";

DeviceConfig readConfigFromPreferences(Preferences& preferences) {
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

    return config;
}

bool writeConfigToPreferences(
    Preferences& preferences,
    const DeviceConfig& config
) {
    const size_t nameWritten =
        preferences.putString("name", config.deviceName);
    const size_t versionWritten =
        preferences.putUInt("version", config.configVersion);
    const size_t intervalWritten = preferences.putUInt(
        "interval",
        config.measurementIntervalSeconds
    );

    return nameWritten == config.deviceName.length() &&
        versionWritten == sizeof(config.configVersion) &&
        intervalWritten == sizeof(config.measurementIntervalSeconds);
}

DeviceConfig loadConfig() {
    Preferences preferences;
    preferences.begin(SENSOR_NAMESPACE, false);

    // Change interval for testing purposes
    // preferences.putUInt("interval", 10);

    DeviceConfig config = readConfigFromPreferences(preferences);

    preferences.end();

    return config;
}

bool saveConfig(const DeviceConfig& config) {
    Preferences preferences;

    if (!preferences.begin(SENSOR_NAMESPACE, false)) {
        LOG_PRINTLN("ERROR: Failed to open configuration storage.");
        return false;
    }

    const DeviceConfig previousConfig = readConfigFromPreferences(preferences);
    const bool success = writeConfigToPreferences(preferences, config);

    if (!success) {
        const bool rollbackSucceeded =
            writeConfigToPreferences(preferences, previousConfig);

        if (!rollbackSucceeded) {
            LOG_PRINTLN(
                "ERROR: Configuration persistence failed and rollback did not complete."
            );
        } else {
            LOG_PRINTLN("ERROR: Configuration persistence failed.");
        }
    }

    preferences.end();

    return success;
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