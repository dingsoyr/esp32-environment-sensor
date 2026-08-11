#include <unity.h>

#include "server_api_protocol.h"

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
    RUN_TEST(test_parses_success_response_without_configuration_update);
    RUN_TEST(test_parses_success_response_with_configuration_update);
    RUN_TEST(test_rejects_partial_configuration_payload);
    RUN_TEST(test_rejects_invalid_server_time);
    return UNITY_END();
}