#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "measurement.h"
#include "storage.h"

Adafruit_BME280 bme;

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
        getNextSequence(),
        bme.readTemperature(),
        bme.readHumidity(),
        bme.readPressure() / 100.0F
    };
}

void printMeasurement(const Measurement& measurement) {
    Serial.println("=== Measurement ===");
    Serial.printf("Sequence:    %lu\n", measurement.sequence);
    Serial.printf("Temperature: %.2f C\n", measurement.temperatureC);
    Serial.printf("Humidity:    %.2f %%\n", measurement.humidityPercent);
    Serial.printf("Pressure:    %.2f hPa\n", measurement.pressureHpa);
    Serial.println("===================");
}