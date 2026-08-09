#pragma once

#include <stdint.h>

uint32_t getCurrentUnixTimestamp();
bool isSystemTimeValid();
bool syncTimeWithNtp();
void setSystemTimeFromUnixTimestamp(uint32_t unixTimestamp);