#include <unity.h>

#include <string>

#include "measurement.h"
#include "measurement_upload_json.h"
#include "storage.h"
#include "server_api_protocol.h"

void test_measurement_layout_includes_battery_fields() {
    const Measurement measurement = {
        1234,
        1786550000,
        21.4F,
        44.2F,
        1013.1F,
        4.05F,
        85,
        true
    };

    TEST_ASSERT_EQUAL_UINT32(1234, measurement.sequence);
    TEST_ASSERT_EQUAL_UINT32(1786550000, measurement.unixTimestamp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 4.05F, measurement.batteryVoltage);
    TEST_ASSERT_EQUAL_UINT8(85, measurement.batteryPercent);
}

void test_measurement_size_matches_esp32_layout_contract() {
    TEST_ASSERT_EQUAL_UINT32(28, sizeof(Measurement));
}

void test_buffer_constants_match_expected_contract() {
    TEST_ASSERT_EQUAL_UINT32(100, MEASUREMENT_BUFFER_CAPACITY);
    TEST_ASSERT_EQUAL_UINT32(3, MEASUREMENT_BUFFER_FORMAT_VERSION);
}

void test_upload_json_serializes_battery_per_measurement_only() {
    const Measurement measurements[] = {
        {
            1234,
            1786550000,
            21.4F,
            44.2F,
            1013.1F,
            4.05F,
            85,
            true
        }
    };

    const std::string json = buildMeasurementUploadRequestJson(
        1,
        "sensor-example",
        "0.1.1-dev",
        5,
        -61,
        measurements,
        1
    );

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        json.find("\"measurements\":[{\"sequence\":1234")
    );
    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        json.find("\"battery_voltage\":4.05")
    );
    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        json.find("\"battery_percent\":85")
    );
    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        json.find("\"status\":{\"rssi_dbm\":-61}")
    );
    TEST_ASSERT_EQUAL(
        std::string::npos,
        json.find("\"status\":{\"rssi_dbm\":-61,\"battery_voltage\"")
    );
    TEST_ASSERT_EQUAL(
        std::string::npos,
        json.find("\"status\":{\"battery_voltage\"")
    );
    TEST_ASSERT_EQUAL(
        std::string::npos,
        json.find("\"status\":{\"battery_percent\"")
    );
}

void test_parses_success_response_without_configuration_update() {
    const char json[] =
        "{"
        "\"api_version\":1,"
        "\"acknowledged_through\":721,"
        "\"server_time\":1786300053,"
        "\"config_version\":3"
        "}";

    MeasurementUploadResponse response;
    const MeasurementUploadResponseParseError error =
        parseMeasurementUploadResponseJson(json, response);

    TEST_ASSERT_EQUAL(
        static_cast<int>(MeasurementUploadResponseParseError::None),
        static_cast<int>(error)
    );
    TEST_ASSERT_EQUAL_UINT32(721, response.acknowledgedThrough);
    TEST_ASSERT_TRUE(response.hasServerTime);
    TEST_ASSERT_EQUAL_UINT32(1786300053, response.serverTime);
    TEST_ASSERT_FALSE(response.hasConfiguration);
    TEST_ASSERT_EQUAL_UINT32(3, response.configVersion);
}

void test_parses_success_response_with_configuration_update() {
    const char json[] =
        "{"
        "\"api_version\":1,"
        "\"acknowledged_through\":721,"
        "\"server_time\":1786300053,"
        "\"config_version\":3,"
        "\"configuration\":{"
        "\"device_name\":\"Utesensor nord\","
        "\"measurement_interval_seconds\":3600"
        "}"
        "}";

    MeasurementUploadResponse response;
    const MeasurementUploadResponseParseError error =
        parseMeasurementUploadResponseJson(json, response);

    TEST_ASSERT_EQUAL(
        static_cast<int>(MeasurementUploadResponseParseError::None),
        static_cast<int>(error)
    );
    TEST_ASSERT_TRUE(response.hasConfiguration);
    TEST_ASSERT_EQUAL_STRING(
        "Utesensor nord",
        response.configuration.deviceName.c_str()
    );
    TEST_ASSERT_EQUAL_UINT32(
        3600,
        response.configuration.measurementIntervalSeconds
    );
}

void test_rejects_partial_configuration_payload() {
    const char json[] =
        "{"
        "\"api_version\":1,"
        "\"acknowledged_through\":721,"
        "\"server_time\":1786300053,"
        "\"config_version\":3,"
        "\"configuration\":{"
        "\"device_name\":\"Utesensor nord\""
        "}"
        "}";

    MeasurementUploadResponse response;
    const MeasurementUploadResponseParseError error =
        parseMeasurementUploadResponseJson(json, response);

    TEST_ASSERT_EQUAL(
        static_cast<int>(
            MeasurementUploadResponseParseError::MissingOrInvalidConfiguration
        ),
        static_cast<int>(error)
    );
}

void test_rejects_invalid_server_time() {
    const char json[] =
        "{"
        "\"api_version\":1,"
        "\"acknowledged_through\":721,"
        "\"server_time\":1234,"
        "\"config_version\":3"
        "}";

    MeasurementUploadResponse response;
    const MeasurementUploadResponseParseError error =
        parseMeasurementUploadResponseJson(json, response);

    TEST_ASSERT_EQUAL(
        static_cast<int>(
            MeasurementUploadResponseParseError::MissingOrInvalidServerTime
        ),
        static_cast<int>(error)
    );
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_measurement_layout_includes_battery_fields);
    RUN_TEST(test_measurement_size_matches_esp32_layout_contract);
    RUN_TEST(test_buffer_constants_match_expected_contract);
    RUN_TEST(test_upload_json_serializes_battery_per_measurement_only);
    RUN_TEST(test_parses_success_response_without_configuration_update);
    RUN_TEST(test_parses_success_response_with_configuration_update);
    RUN_TEST(test_rejects_partial_configuration_payload);
    RUN_TEST(test_rejects_invalid_server_time);
    return UNITY_END();
}