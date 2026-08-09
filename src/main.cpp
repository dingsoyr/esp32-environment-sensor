#include <Preferences.h>
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>
#include <type_traits>

#include "secrets.h"

Adafruit_BME280 bme;

constexpr uint32_t WIFI_TIMEOUT_MS = 10000;
constexpr uint32_t MEASUREMENT_BUFFER_CAPACITY = 16;
constexpr char SENSOR_NAMESPACE[] = "sensor";
constexpr char BUFFER_NAMESPACE[] = "buffer";
constexpr char QUEUED_COUNT_KEY[] = "count";
constexpr char OLDEST_INDEX_KEY[] = "oldest";
constexpr char NEXT_WRITE_INDEX_KEY[] = "next";

struct DeviceConfig {
    String deviceName;
    uint32_t configVersion;
    uint32_t measurementIntervalSeconds;
};

struct Measurement {
    uint32_t sequence;
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
};

struct MeasurementBufferState {
    uint32_t queuedCount;
    uint32_t oldestIndex;
    uint32_t nextWriteIndex;
};

static_assert(
    std::is_trivially_copyable<Measurement>::value,
    "Measurement must remain trivially copyable for NVS storage."
);

void makeMeasurementKey(uint32_t slotIndex, char* key, size_t keySize) {
    snprintf(key, keySize, "m%lu", slotIndex);
}

bool initSensor() {
    Wire.begin(21, 22);

    if (bme.begin(0x76)) {
        return true;
    }

    if (bme.begin(0x77)) {
        return true;
    }

    return false;
}

DeviceConfig loadConfig() {
    Preferences preferences;
    preferences.begin(SENSOR_NAMESPACE, false);

    // TEMP: force a 10 second persisted interval for ring-buffer testing.
    preferences.putUInt("interval", 10);

    DeviceConfig config;

    config.deviceName =
        preferences.isKey("name")
            ? preferences.getString("name")
            : "unnamed";

    config.configVersion =
        preferences.isKey("version")
            ? preferences.getUInt("version")
            : 1;

    config.measurementIntervalSeconds =
        preferences.isKey("interval")
            ? preferences.getUInt("interval")
            : 30;

    preferences.end();

    return config;
}

void printConfig(const DeviceConfig& config) {
    Serial.println("=== Configuration ===");
    Serial.printf("Device name: %s\n", config.deviceName.c_str());
    Serial.printf("Config version: %lu\n", config.configVersion);
    Serial.printf(
        "Measurement interval: %lu seconds\n",
        config.measurementIntervalSeconds
    );
    Serial.println("=====================");
}

uint32_t getNextSequence() {
    Preferences preferences;
    preferences.begin(SENSOR_NAMESPACE, false);

    uint32_t sequence =
        preferences.isKey("sequence")
            ? preferences.getUInt("sequence")
            : 0;

    sequence++;

    preferences.putUInt("sequence", sequence);
    preferences.end();

    return sequence;
}

MeasurementBufferState loadMeasurementBufferState(Preferences& preferences) {
    return {
        preferences.isKey(QUEUED_COUNT_KEY)
            ? preferences.getUInt(QUEUED_COUNT_KEY)
            : 0,
        preferences.isKey(OLDEST_INDEX_KEY)
            ? preferences.getUInt(OLDEST_INDEX_KEY)
            : 0,
        preferences.isKey(NEXT_WRITE_INDEX_KEY)
            ? preferences.getUInt(NEXT_WRITE_INDEX_KEY)
            : 0
    };
}

void saveMeasurementBufferState(
    Preferences& preferences,
    const MeasurementBufferState& state
) {
    preferences.putUInt(QUEUED_COUNT_KEY, state.queuedCount);
    preferences.putUInt(OLDEST_INDEX_KEY, state.oldestIndex);
    preferences.putUInt(NEXT_WRITE_INDEX_KEY, state.nextWriteIndex);
}

void storeMeasurement(const Measurement& measurement) {
    Preferences preferences;
    preferences.begin(BUFFER_NAMESPACE, false);

    MeasurementBufferState state = loadMeasurementBufferState(preferences);
    const bool isBufferFull = state.queuedCount >= MEASUREMENT_BUFFER_CAPACITY;

    if (state.queuedCount > MEASUREMENT_BUFFER_CAPACITY) {
        state.queuedCount = MEASUREMENT_BUFFER_CAPACITY;
    }

    if (state.oldestIndex >= MEASUREMENT_BUFFER_CAPACITY) {
        state.oldestIndex = 0;
    }

    if (state.nextWriteIndex >= MEASUREMENT_BUFFER_CAPACITY) {
        state.nextWriteIndex = 0;
    }

    if (isBufferFull) {
        char discardedKey[16];
        makeMeasurementKey(state.oldestIndex, discardedKey, sizeof(discardedKey));

        Measurement discardedMeasurement;
        const size_t discardedBytes =
            preferences.getBytes(
                discardedKey,
                &discardedMeasurement,
                sizeof(discardedMeasurement)
            );

        if (discardedBytes == sizeof(discardedMeasurement)) {
            Serial.printf(
                "Buffer full. Discarding oldest buffered measurement %lu.\n",
                discardedMeasurement.sequence
            );
        } else {
            Serial.printf(
                "Buffer full. Discarding oldest buffered measurement in slot %lu.\n",
                state.oldestIndex
            );
        }
    }

    const uint32_t writeIndex =
        isBufferFull ? state.oldestIndex : state.nextWriteIndex;

    char key[16];
    makeMeasurementKey(writeIndex, key, sizeof(key));

    const size_t written =
        preferences.putBytes(key, &measurement, sizeof(measurement));

    if (written != sizeof(measurement)) {
        Serial.printf(
            "ERROR: Failed to store measurement %lu.\n",
            measurement.sequence
        );
        preferences.end();
        return;
    }

    if (!isBufferFull) {
        state.queuedCount++;
        state.nextWriteIndex =
            (state.nextWriteIndex + 1) % MEASUREMENT_BUFFER_CAPACITY;
    } else {
        state.oldestIndex =
            (state.oldestIndex + 1) % MEASUREMENT_BUFFER_CAPACITY;
        state.nextWriteIndex = state.oldestIndex;
    }

    saveMeasurementBufferState(preferences, state);
    preferences.end();
}

void printQueuedMeasurements() {
    Preferences preferences;
    preferences.begin(BUFFER_NAMESPACE, true);

    MeasurementBufferState state = loadMeasurementBufferState(preferences);

    Serial.println("=== Buffered Measurements ===");
    Serial.printf("Queued count: %lu\n", state.queuedCount);
    Serial.print("Sequences: ");

    bool printedAny = false;

    for (uint32_t offset = 0; offset < state.queuedCount; ++offset) {
        const uint32_t sequenceIndex =
            (state.oldestIndex + offset) % MEASUREMENT_BUFFER_CAPACITY;

        char key[16];
        makeMeasurementKey(sequenceIndex, key, sizeof(key));

        if (!preferences.isKey(key)) {
            continue;
        }

        Measurement bufferedMeasurement;
        const size_t bytesRead =
            preferences.getBytes(key, &bufferedMeasurement, sizeof(bufferedMeasurement));

        if (bytesRead != sizeof(bufferedMeasurement)) {
            continue;
        }

        if (printedAny) {
            Serial.print(", ");
        }

        Serial.print(bufferedMeasurement.sequence);
        printedAny = true;
    }

    if (!printedAny) {
        Serial.print("none");
    }

    Serial.println();
    Serial.println("=============================");

    preferences.end();
}

Measurement readMeasurement() {
    return {
        getNextSequence(),
        bme.readTemperature(),
        bme.readHumidity(),
        bme.readPressure() / 100.0F
    };
}

void printMeasurement(const Measurement& measurement) {
    Serial.println("=== Measurement ===");
    Serial.printf("Sequence:    %lu\n", measurement.sequence);
    Serial.printf("Temperature: %.2f C\n", measurement.temperatureC);
    Serial.printf("Humidity:    %.2f %%\n", measurement.humidityPercent);
    Serial.printf("Pressure:    %.2f hPa\n", measurement.pressureHpa);
    Serial.println("===================");
}

bool connectWifi() {
    Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);

    const unsigned long start = millis();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            Serial.println("Wi-Fi connection timed out.");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            return false;
        }

        delay(100);
    }

    const unsigned long duration = millis() - start;

    Serial.println("Wi-Fi connected.");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Connection time: %lu ms\n", duration);

    return true;
}

void disconnectWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.println("Wi-Fi disconnected.");
}

void goToSleep(uint32_t sleepDurationSeconds) {
    Serial.printf(
        "Going to deep sleep for %lu seconds...\n",
        sleepDurationSeconds
    );

    Serial.flush();

    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(sleepDurationSeconds) * 1000000ULL
    );

    esp_deep_sleep_start();
}

String getDeviceId() {
    uint64_t chipId = ESP.getEfuseMac();

    char deviceId[20];

    snprintf(
        deviceId,
        sizeof(deviceId),
        "sensor-%06llx",
        chipId & 0xFFFFFF
    );

    return String(deviceId);
}

void setup() {
    Serial.begin(115200);

    const unsigned long awakeStart = millis();

    Serial.println();
    Serial.println("ESP32 environment sensor starting...");

    String deviceId = getDeviceId();
    Serial.printf("Device ID: %s\n", deviceId.c_str());

    DeviceConfig config = loadConfig();
    printConfig(config);

    if (!initSensor()) {
        Serial.println("ERROR: BME280 not found.");
        goToSleep(config.measurementIntervalSeconds);
    }

    Measurement measurement = readMeasurement();
    printMeasurement(measurement);
    storeMeasurement(measurement);
    printQueuedMeasurements();

    if (connectWifi()) {
        disconnectWifi();
    }

    Serial.printf("Total awake time: %lu ms\n", millis() - awakeStart);

    goToSleep(config.measurementIntervalSeconds);
}

void loop() {
}