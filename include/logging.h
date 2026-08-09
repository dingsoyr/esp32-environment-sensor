#pragma once

#include <Arduino.h>

#ifndef DEBUG_LOGGING
#define DEBUG_LOGGING 0
#endif

#if DEBUG_LOGGING
#define LOG_BEGIN(...) Serial.begin(__VA_ARGS__)
#define LOG_PRINT(...) Serial.print(__VA_ARGS__)
#define LOG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define LOG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define LOG_FLUSH() Serial.flush()
#else
#define LOG_BEGIN(...) ((void)0)
#define LOG_PRINT(...) ((void)0)
#define LOG_PRINTLN(...) ((void)0)
#define LOG_PRINTF(...) ((void)0)
#define LOG_FLUSH() ((void)0)
#endif