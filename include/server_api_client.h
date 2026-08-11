#pragma once

#include <Arduino.h>

#include "config.h"

struct MeasurementUploadOutcome {
    bool attempted;
    bool succeeded;
    bool configUpdated;
    DeviceConfig effectiveConfig;
};

MeasurementUploadOutcome uploadBufferedMeasurements(
    const String& deviceId,
    const DeviceConfig& currentConfig,
    const char* measurementsUrl
);