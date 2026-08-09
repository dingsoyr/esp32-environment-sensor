#include <Arduino.h>
#include "config.h"
#include "device.h"
#include "measurement.h"
#include "power.h"
#include "storage.h"
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

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);
    storeMeasurement(measurement);
    printQueuedMeasurements();

    if (connectWifi()) {
        disconnectWifi();
    }

    Serial.printf("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep(config.measurementIntervalSeconds);
}

void loop() {
}