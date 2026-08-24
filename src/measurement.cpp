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

#if DEBUG_LOGGING
void logI2cDevices() {
    bool foundAny = false;

    LOG_PRINTLN("Scanning I2C bus...");

    for (uint8_t address = 1; address < 0x78; ++address) {
        Wire.beginTransmission(address);

        if (Wire.endTransmission() == 0) {
            LOG_PRINTF("Found I2C device at 0x%02X\n", address);
            foundAny = true;
        }
    }

    if (!foundAny) {
        LOG_PRINTLN("No I2C devices found.");
    }
}
#endif

}  // namespace

bool initSensor() {
    Wire.begin();

    if (bme.begin(0x76)) {
        return true;
    }

    if (bme.begin(0x77)) {
        return true;
    }

#if DEBUG_LOGGING
    logI2cDevices();
#endif

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