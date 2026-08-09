#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>

#include "secrets.h"

Adafruit_BME280 bme;

constexpr uint64_t SLEEP_DURATION_SECONDS = 30;
constexpr uint32_t WIFI_TIMEOUT_MS = 10000;

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

void goToSleep() {
    Serial.printf(
        "Going to deep sleep for %llu seconds...\n",
        SLEEP_DURATION_SECONDS
    );

    Serial.flush();

    esp_sleep_enable_timer_wakeup(
        SLEEP_DURATION_SECONDS * 1000000ULL
    );

    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);

    const unsigned long awakeStart = millis();

    Serial.println();
    Serial.println("ESP32 environment sensor starting...");

    if (!initSensor()) {
        Serial.println("ERROR: BME280 not found.");
        goToSleep();
    }

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);

    if (connectWifi()) {
        disconnectWifi();
    }

    Serial.printf("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep();
}

void loop() {
}