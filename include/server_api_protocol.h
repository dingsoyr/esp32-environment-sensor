#pragma once

#include <stdint.h>

#include <string>

constexpr uint32_t ENVIRONMENT_SENSOR_SERVER_API_VERSION = 1;

struct ServerConfigurationPayload {
    std::string deviceName;
    uint32_t measurementIntervalSeconds;
};

struct MeasurementUploadResponse {
    uint32_t acknowledgedThrough;
    uint32_t configVersion;
    uint32_t serverTime;
    bool hasServerTime;
    bool hasConfiguration;
    ServerConfigurationPayload configuration;
};

enum class MeasurementUploadResponseParseError {
    None,
    InvalidJson,
    MissingOrInvalidApiVersion,
    MissingOrInvalidAcknowledgedThrough,
    MissingOrInvalidServerTime,
    MissingOrInvalidConfigVersion,
    MissingOrInvalidConfiguration
};

MeasurementUploadResponseParseError parseMeasurementUploadResponseJson(
    const char* json,
    MeasurementUploadResponse& response
);

const char* measurementUploadResponseParseErrorToString(
    MeasurementUploadResponseParseError error
);