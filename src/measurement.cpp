#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "logging.h"
#include "measurement.h"
#include "storage.h"
#include "time_utils.h"

Adafruit_BME280 bme;

namespace {

constexpr float TEMPORARY_BATTERY_VOLTAGE_VOLTS = 4.05F;
constexpr uint8_t TEMPORARY_BATTERY_PERCENT = 85;

}  // namespace

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
    const bool timestampValid = isSystemTimeValid();

    return {
        getNextSequence(),
        timestampValid ? getCurrentUnixTimestamp() : 0,
        bme.readTemperature(),
        bme.readHumidity(),
        bme.readPressure() / 100.0F,
        TEMPORARY_BATTERY_VOLTAGE_VOLTS,
        TEMPORARY_BATTERY_PERCENT,
        timestampValid
    };
}

void printMeasurement(const Measurement& measurement) {
    LOG_PRINTLN("=== Measurement ===");
    LOG_PRINTF("Sequence:    %lu\n", measurement.sequence);
    LOG_PRINTF("Temperature: %.2f C\n", measurement.temperatureC);
    LOG_PRINTF("Humidity:    %.2f %%\n", measurement.humidityPercent);
    LOG_PRINTF("Pressure:    %.2f hPa\n", measurement.pressureHpa);
    LOG_PRINTF(
        "Battery:     %u %% / %.2f V\n",
        measurement.batteryPercent,
        measurement.batteryVoltage
    );

    if (measurement.timestampValid) {
        LOG_PRINTF("Timestamp:   %lu (UTC Unix)\n", measurement.unixTimestamp);
    } else {
        LOG_PRINTLN("Timestamp:   invalid");
    }

    LOG_PRINTLN("===================");
}