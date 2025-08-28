/**
 * Debug Utilities
 * 
 * Simple debug output macros for development and troubleshooting.
 * Can be easily disabled in production builds by commenting out DEBUG_ENABLED.
 * 
 * @author Stephane Bhiri
 * @version 2.0
 * @date January 2025
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Enable/disable debug output
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  #define Debug(...) do { Serial.printf(__VA_ARGS__); } while(0)
#else
  #define Debug(...)
#endif

#endif