#include <Preferences.h>
#include <type_traits>

#include "storage.h"

constexpr uint32_t MEASUREMENT_BUFFER_CAPACITY = 16;
constexpr char SENSOR_NAMESPACE[] = "sensor";
constexpr char BUFFER_NAMESPACE[] = "buffer";
constexpr char QUEUED_COUNT_KEY[] = "count";
constexpr char OLDEST_INDEX_KEY[] = "oldest";
constexpr char NEXT_WRITE_INDEX_KEY[] = "next";

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

bool loadBufferedMeasurement(
    Preferences& preferences,
    uint32_t slotIndex,
    Measurement& measurement
) {
    char key[16];
    makeMeasurementKey(slotIndex, key, sizeof(key));

    if (!preferences.isKey(key)) {
        return false;
    }

    return preferences.getBytes(key, &measurement, sizeof(measurement)) ==
        sizeof(measurement);
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
        Measurement discardedMeasurement;
        if (loadBufferedMeasurement(
            preferences,
            state.oldestIndex,
            discardedMeasurement
        )) {
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

void acknowledgeMeasurementsUpTo(uint32_t acknowledgedSequence) {
    Preferences preferences;
    preferences.begin(BUFFER_NAMESPACE, false);

    MeasurementBufferState state = loadMeasurementBufferState(preferences);
    uint32_t removedCount = 0;

    while (state.queuedCount > 0) {
        Measurement oldestMeasurement;

        if (!loadBufferedMeasurement(
            preferences,
            state.oldestIndex,
            oldestMeasurement
        )) {
            Serial.printf(
                "ACK stopped. Missing buffered measurement in slot %lu.\n",
                state.oldestIndex
            );
            break;
        }

        if (oldestMeasurement.sequence > acknowledgedSequence) {
            break;
        }

        char key[16];
        makeMeasurementKey(state.oldestIndex, key, sizeof(key));
        preferences.remove(key);

        state.oldestIndex =
            (state.oldestIndex + 1) % MEASUREMENT_BUFFER_CAPACITY;
        state.queuedCount--;
        removedCount++;
    }

    if (state.queuedCount == 0) {
        state.nextWriteIndex = state.oldestIndex;
    }

    saveMeasurementBufferState(preferences, state);
    preferences.end();

    Serial.printf(
        "ACK up to %lu removed %lu buffered measurement(s).\n",
        acknowledgedSequence,
        removedCount
    );
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

        Measurement bufferedMeasurement;
        if (!loadBufferedMeasurement(
            preferences,
            sequenceIndex,
            bufferedMeasurement
        )) {
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