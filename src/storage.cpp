#include <Preferences.h>
#include <type_traits>

#include "storage.h"

constexpr uint32_t MEASUREMENT_BUFFER_CAPACITY = 100;
constexpr uint32_t MEASUREMENT_BUFFER_FORMAT_VERSION = 2;
constexpr char SENSOR_NAMESPACE[] = "sensor";
constexpr char BUFFER_NAMESPACE[] = "buffer";
constexpr char QUEUED_COUNT_KEY[] = "count";
constexpr char OLDEST_INDEX_KEY[] = "oldest";
constexpr char NEXT_WRITE_INDEX_KEY[] = "next";
constexpr char BUFFER_FORMAT_KEY[] = "format";

struct MeasurementBufferState {
    uint32_t queuedCount;
    uint32_t oldestIndex;
    uint32_t nextWriteIndex;
};

MeasurementBufferState loadMeasurementBufferState(Preferences& preferences);
void saveMeasurementBufferState(
    Preferences& preferences,
    const MeasurementBufferState& state
);
bool loadBufferedMeasurement(
    Preferences& preferences,
    uint32_t slotIndex,
    Measurement& measurement
);

static_assert(
    std::is_trivially_copyable<Measurement>::value,
    "Measurement must remain trivially copyable for NVS storage."
);

void makeMeasurementKey(uint32_t slotIndex, char* key, size_t keySize) {
    snprintf(key, keySize, "m%lu", slotIndex);
}

MeasurementBufferState sanitizeMeasurementBufferState(
    const MeasurementBufferState& state,
    uint32_t capacity
) {
    MeasurementBufferState sanitizedState = state;

    if (capacity == 0) {
        return {0, 0, 0};
    }

    if (sanitizedState.queuedCount > capacity) {
        sanitizedState.queuedCount = capacity;
    }

    if (sanitizedState.oldestIndex >= capacity) {
        sanitizedState.oldestIndex = 0;
    }

    if (sanitizedState.nextWriteIndex >= capacity) {
        sanitizedState.nextWriteIndex = 0;
    }

    return sanitizedState;
}

void clearBufferedMeasurements(Preferences& preferences, uint32_t slotCount) {
    preferences.remove(QUEUED_COUNT_KEY);
    preferences.remove(OLDEST_INDEX_KEY);
    preferences.remove(NEXT_WRITE_INDEX_KEY);

    for (uint32_t slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        char key[16];
        makeMeasurementKey(slotIndex, key, sizeof(key));

        if (preferences.isKey(key)) {
            preferences.remove(key);
        }
    }
}

void clearBufferedMeasurements(Preferences& preferences) {
    clearBufferedMeasurements(preferences, MEASUREMENT_BUFFER_CAPACITY);
}

void ensureMeasurementBufferFormat(Preferences& preferences) {
    const uint32_t storedFormat =
        preferences.isKey(BUFFER_FORMAT_KEY)
            ? preferences.getUInt(BUFFER_FORMAT_KEY)
            : 0;

    if (storedFormat == MEASUREMENT_BUFFER_FORMAT_VERSION) {
        if (preferences.isKey("capacity")) {
            const uint32_t storedSlotCount = preferences.getUInt("capacity");

            clearBufferedMeasurements(
                preferences,
                storedSlotCount > MEASUREMENT_BUFFER_CAPACITY
                    ? storedSlotCount
                    : MEASUREMENT_BUFFER_CAPACITY
            );
            saveMeasurementBufferState(preferences, {0, 0, 0});
            preferences.remove("capacity");

            Serial.println(
                "Buffered measurement reset after removing capacity-migration metadata."
            );
        }

        return;
    }

    if (storedFormat == 0) {
        Serial.println(
            "Buffered measurement format upgrade detected. Clearing old test buffer because previous records use the old binary Measurement layout."
        );
    } else {
        Serial.printf(
            "Unsupported buffered measurement format %lu. Clearing buffered measurements.\n",
            storedFormat
        );
    }

    clearBufferedMeasurements(preferences, MEASUREMENT_BUFFER_CAPACITY);
    preferences.putUInt(BUFFER_FORMAT_KEY, MEASUREMENT_BUFFER_FORMAT_VERSION);
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

    if (preferences.getBytesLength(key) != sizeof(measurement)) {
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
    ensureMeasurementBufferFormat(preferences);

    MeasurementBufferState state = sanitizeMeasurementBufferState(
        loadMeasurementBufferState(preferences),
        MEASUREMENT_BUFFER_CAPACITY
    );
    const bool isBufferFull = state.queuedCount >= MEASUREMENT_BUFFER_CAPACITY;

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
    ensureMeasurementBufferFormat(preferences);

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
    preferences.begin(BUFFER_NAMESPACE, false);
    ensureMeasurementBufferFormat(preferences);

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
