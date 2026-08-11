#ifndef WIFI_BACKOFF_H
#define WIFI_BACKOFF_H

#include <Arduino.h>

/// Exponential reconnect backoff with jitter — pure and host-testable.
///
/// base = 1000 ms << min(attempt, 5), capped at 30000 ms; a jitter term of
/// up to 25 % of the base is added so that several taps that lose the AP
/// simultaneously do not all retry at the same instant (FW-1 / E6). The cap
/// plus WiFiHandler's FATAL_TIMEOUT_MS guarantee the wrong-AP case terminates
/// instead of oscillating forever.
static inline unsigned long wifiBackoffMs(unsigned int attempt) {
    const unsigned long BASE_MS   = 1000;
    const unsigned long MAX_MS    = 30000;
    const unsigned int  MAX_SHIFT = 5;          // 1 << 5 = 32 s base -> capped

    unsigned int  shift = (attempt < MAX_SHIFT) ? attempt : MAX_SHIFT;
    unsigned long base  = BASE_MS << shift;
    if (base > MAX_MS) {
        base = MAX_MS;
    }

    unsigned long jitter = random(0, base / 4 + 1);   // [0, base/4]
    return base + jitter;
}

#endif // WIFI_BACKOFF_H
