#pragma once

#include <stdint.h>

#include "measurement.h"

uint32_t getNextSequence();
void storeMeasurement(const Measurement& measurement);
void acknowledgeMeasurementsUpTo(uint32_t acknowledgedSequence);
void printQueuedMeasurements();