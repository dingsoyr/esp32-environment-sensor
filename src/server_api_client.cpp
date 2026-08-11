#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "config.h"
#include "logging.h"
#include "server_api_client.h"
#include "server_api_protocol.h"
#include "storage.h"
#include "time_utils.h"
#include "wifi_utils.h"

namespace {

constexpr char FIRMWARE_VERSION[] = "0.1.0-dev";
constexpr uint32_t HTTP_TIMEOUT_MS = 5000;

String buildMeasurementUploadRequest(
    const String& deviceId,
    const DeviceConfig& config,
    int32_t rssiDbm,
    const Measurement* measurements,
    uint32_t measurementCount
) {
    String requestBody;
    requestBody.reserve(256 + measurementCount * 160);

    requestBody += "{\"api_version\":";
    requestBody += String(ENVIRONMENT_SENSOR_SERVER_API_VERSION);
    requestBody += ",\"device_id\":\"";
    requestBody += deviceId;
    requestBody += "\",\"firmware_version\":\"";
    requestBody += FIRMWARE_VERSION;
    requestBody += "\",\"config_version\":";
    requestBody += String(config.configVersion);
    requestBody += ",\"status\":{\"rssi_dbm\":";
    requestBody += String(rssiDbm);
    requestBody += "},\"measurements\":[";

    for (uint32_t index = 0; index < measurementCount; ++index) {
        if (index > 0) {
            requestBody += ',';
        }

        const Measurement& measurement = measurements[index];

        requestBody += "{\"sequence\":";
        requestBody += String(measurement.sequence);
        requestBody += ",\"measured_at\":";
        requestBody += String(measurement.unixTimestamp);
        requestBody += ",\"timestamp_valid\":";
        requestBody += measurement.timestampValid ? "true" : "false";
        requestBody += ",\"temperature_c\":";
        requestBody += String(measurement.temperatureC, 2);
        requestBody += ",\"humidity_percent\":";
        requestBody += String(measurement.humidityPercent, 2);
        requestBody += ",\"pressure_hpa\":";
        requestBody += String(measurement.pressureHpa, 2);
        requestBody += '}';
    }

    requestBody += "]}";

    return requestBody;
}

void logHttpFailure(int httpStatus) {
    if (httpStatus < 0) {
        LOG_PRINTF(
            "Measurement upload failed: %s\n",
            HTTPClient::errorToString(httpStatus).c_str()
        );
        return;
    }

    LOG_PRINTF(
        "Measurement upload failed: unexpected HTTP status %d\n",
        httpStatus
    );
}

}  // namespace

MeasurementUploadOutcome uploadBufferedMeasurements(
    const String& deviceId,
    const DeviceConfig& currentConfig
) {
    MeasurementUploadOutcome outcome = {
        false,
        false,
        false,
        currentConfig
    };

    if (!isWifiConnected()) {
        LOG_PRINTLN("Measurement upload skipped because Wi-Fi is not connected.");
        return outcome;
    }

    Measurement queuedMeasurements[MEASUREMENT_BUFFER_CAPACITY];
    uint32_t queuedCount = 0;

    if (!loadQueuedMeasurements(
        queuedMeasurements,
        MEASUREMENT_BUFFER_CAPACITY,
        &queuedCount
    )) {
        LOG_PRINTLN("Measurement upload skipped because buffered measurements could not be loaded.");
        return outcome;
    }

    if (queuedCount == 0) {
        LOG_PRINTLN("Measurement upload skipped because no buffered measurements are available.");
        return outcome;
    }

    outcome.attempted = true;

    LOG_PRINTF("Uploading %lu buffered measurement(s).\n", queuedCount);

    WiFiClient wifiClient;
    HTTPClient http;

    if (!http.begin(wifiClient, ENVIRONMENT_SENSOR_SERVER_MEASUREMENTS_URL)) {
        LOG_PRINTLN("Measurement upload failed: could not initialize HTTP client.");
        return outcome;
    }

    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

    const String requestBody = buildMeasurementUploadRequest(
        deviceId,
        currentConfig,
        WiFi.RSSI(),
        queuedMeasurements,
        queuedCount
    );
    const int httpStatus = http.POST(requestBody);

    LOG_PRINTF("Measurement upload HTTP status: %d\n", httpStatus);

    if (httpStatus != HTTP_CODE_OK) {
        logHttpFailure(httpStatus);
        http.end();
        return outcome;
    }

    const String responseBody = http.getString();
    http.end();

    MeasurementUploadResponse response;
    const MeasurementUploadResponseParseError parseError =
        parseMeasurementUploadResponseJson(responseBody.c_str(), response);

    if (parseError != MeasurementUploadResponseParseError::None) {
        LOG_PRINTF(
            "Measurement upload failed: %s\n",
            measurementUploadResponseParseErrorToString(parseError)
        );
        return outcome;
    }

    LOG_PRINTF(
        "Measurement upload acknowledged through %lu.\n",
        response.acknowledgedThrough
    );

    if (response.hasServerTime) {
        setSystemTimeFromUnixTimestamp(response.serverTime);
        LOG_PRINTF(
            "System time synchronized from server: %lu\n",
            response.serverTime
        );
    }

    if (response.hasConfiguration) {
        LOG_PRINTF(
            "Received configuration update version %lu.\n",
            response.configVersion
        );

        DeviceConfig updatedConfig = currentConfig;
        updatedConfig.deviceName = response.configuration.deviceName.c_str();
        updatedConfig.measurementIntervalSeconds =
            response.configuration.measurementIntervalSeconds;
        updatedConfig.configVersion = response.configVersion;

        if (saveConfig(updatedConfig)) {
            outcome.configUpdated = true;
            outcome.effectiveConfig = updatedConfig;

            LOG_PRINTF(
                "Configuration update persisted. New interval: %lu seconds\n",
                updatedConfig.measurementIntervalSeconds
            );
        } else {
            LOG_PRINTLN(
                "Configuration update received but could not be persisted."
            );
        }
    } else if (response.configVersion != currentConfig.configVersion) {
        LOG_PRINTF(
            "Server reported config version %lu without configuration payload. Local configuration unchanged.\n",
            response.configVersion
        );
    }

    acknowledgeMeasurementsUpTo(response.acknowledgedThrough);
    outcome.succeeded = true;

    return outcome;
}