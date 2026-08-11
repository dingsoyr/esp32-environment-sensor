#pragma once

#include <stdint.h>

constexpr uint32_t MIN_VALID_UNIX_TIMESTAMP = 1704067200UL;

uint32_t getCurrentUnixTimestamp();
bool isSystemTimeValid();
bool isUnixTimestampValid(uint32_t unixTimestamp);
bool syncTimeWithNtp();
void setSystemTimeFromUnixTimestamp(uint32_t unixTimestamp);