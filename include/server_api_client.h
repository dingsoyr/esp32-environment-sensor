#pragma once

#include <Arduino.h>

#include "config.h"

constexpr char ENVIRONMENT_SENSOR_SERVER_MEASUREMENTS_URL[] =
    "http://192.168.80.140:8000/api/v1/measurements";

struct MeasurementUploadOutcome {
    bool attempted;
    bool succeeded;
    bool configUpdated;
    DeviceConfig effectiveConfig;
};

MeasurementUploadOutcome uploadBufferedMeasurements(
    const String& deviceId,
    const DeviceConfig& currentConfig
);