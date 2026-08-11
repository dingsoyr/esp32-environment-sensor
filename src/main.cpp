#include <Arduino.h>
#include "config.h"
#include "device.h"
#include "logging.h"
#include "measurement.h"
#include "power.h"
#include "server_api_client.h"
#include "storage.h"
#include "time_utils.h"
#include "wifi_utils.h"

void setup() {
    LOG_BEGIN(115200);

    const unsigned long awakeStart = millis();

    LOG_PRINTLN();
    LOG_PRINTLN("ESP32 environment sensor starting...");

    String deviceId = getDeviceId();
    LOG_PRINTF("Device ID: %s\n", deviceId.c_str());

    DeviceConfig config = loadConfig();
    printConfig(config);

    if (!initSensor()) {
        LOG_PRINTLN("ERROR: BME280 not found.");
        goToSleep(config.measurementIntervalSeconds);
    }

    const bool localTimeValidOnWake = isSystemTimeValid();
    bool wifiConnected = isWifiConnected();
    bool ntpSyncAttempted = false;
    bool ntpSyncSucceeded = false;

    LOG_PRINTF(
        "Local time valid on wake: %s\n",
        localTimeValidOnWake ? "yes" : "no"
    );

    if (!localTimeValidOnWake) {
        wifiConnected = connectWifi();

        if (wifiConnected) {
            ntpSyncAttempted = true;
            ntpSyncSucceeded = syncTimeWithNtp();
        } else {
            LOG_PRINTLN(
                "NTP synchronization skipped because Wi-Fi connection failed."
            );
        }
    }

    LOG_PRINTF(
        "NTP synchronization attempted: %s\n",
        ntpSyncAttempted ? "yes" : "no"
    );

    if (ntpSyncAttempted) {
        LOG_PRINTF(
            "NTP synchronization result: %s\n",
            ntpSyncSucceeded ? "succeeded" : "failed or timed out"
        );
    }

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);
    storeMeasurement(measurement);
    printQueuedMeasurements();

    if (!wifiConnected) {
        wifiConnected = connectWifi();
    } else {
        LOG_PRINTLN("Reusing existing Wi-Fi connection for current wake cycle.");
    }

    if (wifiConnected) {
        const MeasurementUploadOutcome uploadOutcome =
            uploadBufferedMeasurements(deviceId, config);

        config = uploadOutcome.effectiveConfig;
        disconnectWifi();
    }

    LOG_PRINTF("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep(config.measurementIntervalSeconds);
}

void loop() {
}