#include <Arduino.h>
#include "config.h"
#include "device.h"
#include "measurement.h"
#include "power.h"
#include "storage.h"
#include "time_utils.h"
#include "wifi_utils.h"

void setup() {
    Serial.begin(115200);

    const unsigned long awakeStart = millis();

    Serial.println();
    Serial.println("ESP32 environment sensor starting...");

    String deviceId = getDeviceId();
    Serial.printf("Device ID: %s\n", deviceId.c_str());

    DeviceConfig config = loadConfig();
    printConfig(config);

    if (!initSensor()) {
        Serial.println("ERROR: BME280 not found.");
        goToSleep(config.measurementIntervalSeconds);
    }

    const bool localTimeValidOnWake = isSystemTimeValid();
    bool wifiConnected = isWifiConnected();
    bool ntpSyncAttempted = false;
    bool ntpSyncSucceeded = false;

    Serial.printf(
        "Local time valid on wake: %s\n",
        localTimeValidOnWake ? "yes" : "no"
    );

    if (!localTimeValidOnWake) {
        wifiConnected = connectWifi();

        if (wifiConnected) {
            ntpSyncAttempted = true;
            ntpSyncSucceeded = syncTimeWithNtp();
        } else {
            Serial.println(
                "NTP synchronization skipped because Wi-Fi connection failed."
            );
        }
    }

    Serial.printf(
        "NTP synchronization attempted: %s\n",
        ntpSyncAttempted ? "yes" : "no"
    );

    if (ntpSyncAttempted) {
        Serial.printf(
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
        Serial.println("Reusing existing Wi-Fi connection for current wake cycle.");
    }

    if (wifiConnected) {
        disconnectWifi();
    }

    Serial.printf("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep(config.measurementIntervalSeconds);
}

void loop() {
}