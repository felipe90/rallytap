#ifndef NETWORK_PROFILES_H
#define NETWORK_PROFILES_H

#include <Arduino.h>

/// Pure multi-AP profile-rotation helpers — host-testable (native env) and
/// used by WiFiHandler (d7a2b5c). A tap rotates through its configured AP
/// profiles (deploy AP -> dev AP) until one links; the pure decision logic
/// lives here so it can be unit-tested without the WiFi/WS hardware seam.

/// Should we rotate to the next profile? Each profile gets a bounded number
/// of backoff ticks before the handler advances (no oscillation on one AP).
static inline bool wifiShouldRotateProfile(unsigned int attempt, unsigned int attemptsPerProfile) {
    return attempt >= attemptsPerProfile;
}

/// Next profile index, wrapping modulo the profile count. count==0 (never
/// happens from begin(), which coerces to 1) degrades to index 0.
static inline size_t wifiNextProfileIndex(size_t currentIndex, size_t profileCount) {
    if (profileCount == 0) return 0;
    return (currentIndex + 1) % profileCount;
}

#endif // NETWORK_PROFILES_H
