#include <Preferences.h>
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>

#include "secrets.h"

Adafruit_BME280 bme;

constexpr uint32_t WIFI_TIMEOUT_MS = 10000;

struct DeviceConfig {
    String deviceName;
    uint32_t configVersion;
    uint32_t measurementIntervalSeconds;
};

struct Measurement {
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
};

bool initSensor() {
    Wire.begin(21, 22);

    if (bme.begin(0x76)) {
        return true;
    }

    if (bme.begin(0x77)) {
        return true;
    }

    return false;
}

DeviceConfig loadConfig() {
    Preferences preferences;
    preferences.begin("sensor", false);

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

Measurement readMeasurement() {
    return {
        bme.readTemperature(),
        bme.readHumidity(),
        bme.readPressure() / 100.0F
    };
}

void printMeasurement(const Measurement& measurement) {
    Serial.println("=== Measurement ===");
    Serial.printf("Temperature: %.2f C\n", measurement.temperatureC);
    Serial.printf("Humidity:    %.2f %%\n", measurement.humidityPercent);
    Serial.printf("Pressure:    %.2f hPa\n", measurement.pressureHpa);
    Serial.println("===================");
}

bool connectWifi() {
    Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);

    const unsigned long start = millis();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            Serial.println("Wi-Fi connection timed out.");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            return false;
        }

        delay(100);
    }

    const unsigned long duration = millis() - start;

    Serial.println("Wi-Fi connected.");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Connection time: %lu ms\n", duration);

    return true;
}

void disconnectWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.println("Wi-Fi disconnected.");
}

void goToSleep(uint32_t sleepDurationSeconds) {
    Serial.printf(
        "Going to deep sleep for %lu seconds...\n",
        sleepDurationSeconds
    );

    Serial.flush();

    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(sleepDurationSeconds) * 1000000ULL
    );

    esp_deep_sleep_start();
}

String getDeviceId() {
    uint64_t chipId = ESP.getEfuseMac();

    char deviceId[20];

    snprintf(
        deviceId,
        sizeof(deviceId),
        "sensor-%06llx",
        chipId & 0xFFFFFF
    );

    return String(deviceId);
}

void saveTestConfig() {
    Preferences preferences;
    preferences.begin("sensor", false);

    preferences.putString("name", "test-sensor");
    preferences.putUInt("version", 2);
    preferences.putUInt("interval", 45);

    preferences.end();

    Serial.println("Test configuration saved.");
}

void setup() {
    Serial.begin(115200);

    const unsigned long awakeStart = millis();

    Serial.println();
    Serial.println("ESP32 environment sensor starting...");

    String deviceId = getDeviceId();
    Serial.printf("Device ID: %s\n", deviceId.c_str());

    // saveTestConfig();

    DeviceConfig config = loadConfig();
    printConfig(config);

    if (!initSensor()) {
        Serial.println("ERROR: BME280 not found.");
        goToSleep(config.measurementIntervalSeconds);
    }

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);

    if (connectWifi()) {
        disconnectWifi();
    }

    Serial.printf("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep(config.measurementIntervalSeconds);
}

void loop() {
}
