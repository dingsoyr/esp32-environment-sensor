#pragma once

#include <stdint.h>

#include <string>

#include "measurement.h"

std::string buildMeasurementUploadRequestJson(
    uint32_t apiVersion,
    const char* deviceId,
    const char* firmwareVersion,
    uint32_t configVersion,
    int32_t rssiDbm,
    const Measurement* measurements,
    uint32_t measurementCount
);