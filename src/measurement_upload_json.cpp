#include "measurement_upload_json.h"

#include <cstdio>

namespace {

constexpr size_t REQUEST_BASE_RESERVE = 256;
constexpr size_t REQUEST_MEASUREMENT_RESERVE = 232;

void appendFormattedFloat(std::string& output, float value) {
    char buffer[32];
    const int length = snprintf(buffer, sizeof(buffer), "%.2f", value);

    if (length > 0) {
        output.append(buffer, static_cast<size_t>(length));
    }
}

void appendMeasurementJson(std::string& output, const Measurement& measurement) {
    output += "{\"sequence\":";
    output += std::to_string(measurement.sequence);
    output += ",\"measured_at\":";
    output += std::to_string(measurement.unixTimestamp);
    output += ",\"timestamp_valid\":";
    output += measurement.timestampValid ? "true" : "false";
    output += ",\"temperature_c\":";
    appendFormattedFloat(output, measurement.temperatureC);
    output += ",\"humidity_percent\":";
    appendFormattedFloat(output, measurement.humidityPercent);
    output += ",\"pressure_hpa\":";
    appendFormattedFloat(output, measurement.pressureHpa);
    output += ",\"battery_voltage\":";
    appendFormattedFloat(output, measurement.batteryVoltage);
    output += ",\"battery_percent\":";
    output += std::to_string(measurement.batteryPercent);
    output += '}';
}

}  // namespace

std::string buildMeasurementUploadRequestJson(
    uint32_t apiVersion,
    const char* deviceId,
    const char* firmwareVersion,
    uint32_t configVersion,
    int32_t rssiDbm,
    const Measurement* measurements,
    uint32_t measurementCount
) {
    std::string requestBody;
    requestBody.reserve(
        REQUEST_BASE_RESERVE +
        static_cast<size_t>(measurementCount) * REQUEST_MEASUREMENT_RESERVE
    );

    requestBody += "{\"api_version\":";
    requestBody += std::to_string(apiVersion);
    requestBody += ",\"device_id\":\"";
    requestBody += deviceId;
    requestBody += "\",\"firmware_version\":\"";
    requestBody += firmwareVersion;
    requestBody += "\",\"config_version\":";
    requestBody += std::to_string(configVersion);
    requestBody += ",\"status\":{\"rssi_dbm\":";
    requestBody += std::to_string(rssiDbm);
    requestBody += "},\"measurements\":[";

    for (uint32_t index = 0; index < measurementCount; ++index) {
        if (index > 0) {
            requestBody += ',';
        }

        appendMeasurementJson(requestBody, measurements[index]);
    }

    requestBody += "]}";

    return requestBody;
}