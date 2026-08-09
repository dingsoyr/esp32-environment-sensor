#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

struct Measurement {
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
};

bool initSensor() {
    Wire.begin(21, 22);  // SDA, SCL

    if (bme.begin(0x76)) {
        return true;
    }

    if (bme.begin(0x77)) {
        return true;
    }

    return false;
}

Measurement readMeasurement() {
    Measurement measurement;

    measurement.temperatureC = bme.readTemperature();
    measurement.humidityPercent = bme.readHumidity();
    measurement.pressureHpa = bme.readPressure() / 100.0F;

    return measurement;
}

void printMeasurement(const Measurement& measurement) {
    Serial.println("=== Measurement ===");

    Serial.printf("Temperature: %.2f C\n", measurement.temperatureC);
    Serial.printf("Humidity:    %.2f %%\n", measurement.humidityPercent);
    Serial.printf("Pressure:    %.2f hPa\n", measurement.pressureHpa);

    Serial.println("===================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("ESP32 environment sensor starting...");

    if (!initSensor()) {
        Serial.println("ERROR: BME280 not found.");
        return;
    }

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);
}

void loop() {
}