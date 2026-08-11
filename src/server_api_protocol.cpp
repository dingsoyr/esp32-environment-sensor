#include <ArduinoJson.h>

#include "server_api_protocol.h"
#include "time_utils.h"

namespace {

bool readUint32(JsonVariantConst value, uint32_t& output) {
    if (!value.is<uint32_t>()) {
        return false;
    }

    output = value.as<uint32_t>();
    return true;
}

}  // namespace

MeasurementUploadResponseParseError parseMeasurementUploadResponseJson(
    const char* json,
    MeasurementUploadResponse& response
) {
    response = {0, 0, 0, false, false, {"", 0}};

    DynamicJsonDocument document(768);
    const DeserializationError error = deserializeJson(document, json);

    if (error) {
        return MeasurementUploadResponseParseError::InvalidJson;
    }

    JsonObjectConst root = document.as<JsonObjectConst>();
    uint32_t apiVersion = 0;

    if (!readUint32(root["api_version"], apiVersion) ||
        apiVersion != ENVIRONMENT_SENSOR_SERVER_API_VERSION) {
        return MeasurementUploadResponseParseError::MissingOrInvalidApiVersion;
    }

    if (!readUint32(
        root["acknowledged_through"],
        response.acknowledgedThrough
    )) {
        return MeasurementUploadResponseParseError::
            MissingOrInvalidAcknowledgedThrough;
    }

    if (!readUint32(root["config_version"], response.configVersion)) {
        return MeasurementUploadResponseParseError::
            MissingOrInvalidConfigVersion;
    }

    if (!root["server_time"].isNull()) {
        if (!readUint32(root["server_time"], response.serverTime) ||
            response.serverTime < MIN_VALID_UNIX_TIMESTAMP) {
            return MeasurementUploadResponseParseError::
                MissingOrInvalidServerTime;
        }

        response.hasServerTime = true;
    }

    if (!root["configuration"].isNull()) {
        JsonObjectConst configuration = root["configuration"].as<JsonObjectConst>();

        if (configuration.isNull() ||
            !configuration["device_name"].is<const char*>()) {
            return MeasurementUploadResponseParseError::
                MissingOrInvalidConfiguration;
        }

        const char* deviceName = configuration["device_name"].as<const char*>();
        uint32_t measurementIntervalSeconds = 0;

        if (deviceName == nullptr || deviceName[0] == '\0' ||
            !readUint32(
                configuration["measurement_interval_seconds"],
                measurementIntervalSeconds
            ) ||
            measurementIntervalSeconds == 0) {
            return MeasurementUploadResponseParseError::
                MissingOrInvalidConfiguration;
        }

        response.configuration.deviceName = deviceName;
        response.configuration.measurementIntervalSeconds =
            measurementIntervalSeconds;
        response.hasConfiguration = true;
    }

    return MeasurementUploadResponseParseError::None;
}

const char* measurementUploadResponseParseErrorToString(
    MeasurementUploadResponseParseError error
) {
    switch (error) {
        case MeasurementUploadResponseParseError::None:
            return "none";
        case MeasurementUploadResponseParseError::InvalidJson:
            return "invalid JSON";
        case MeasurementUploadResponseParseError::MissingOrInvalidApiVersion:
            return "missing or invalid api_version";
        case MeasurementUploadResponseParseError::
            MissingOrInvalidAcknowledgedThrough:
            return "missing or invalid acknowledged_through";
        case MeasurementUploadResponseParseError::MissingOrInvalidServerTime:
            return "missing or invalid server_time";
        case MeasurementUploadResponseParseError::MissingOrInvalidConfigVersion:
            return "missing or invalid config_version";
        case MeasurementUploadResponseParseError::MissingOrInvalidConfiguration:
            return "missing or invalid configuration";
    }

    return "unknown";
}