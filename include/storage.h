#pragma once

#include <stdint.h>

#include "measurement.h"

constexpr uint32_t MEASUREMENT_BUFFER_CAPACITY = 100;

uint32_t getNextSequence();
void storeMeasurement(const Measurement& measurement);
bool loadQueuedMeasurements(
	Measurement* measurements,
	uint32_t capacity,
	uint32_t* loadedCount
);
void acknowledgeMeasurementsUpTo(uint32_t acknowledgedSequence);
void printQueuedMeasurements();