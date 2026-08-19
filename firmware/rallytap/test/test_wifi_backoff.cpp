/**
 * Unit tests for the WiFi reconnect backoff computation (FW-1 / E6).
 *
 * The backoff is a pure function so it can run on the host via [env:native]
 * without the ESP32 WiFi/WebSockets hardware. It is used by WiFiHandler for
 * reconnect attempts: exponential base (1000 ms << attempt) capped at 30 s,
 * plus a ±25 % jitter term so simultaneous taps do not stampede the hub.
 */
#include <Arduino.h>
#include <unity.h>
#include "../include/WifiBackoff.h"

void runTests_wifiBackoff(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// Exponential growth — higher attempt -> strictly larger delay
// ---------------------------------------------------------------------------

void test_backoff_grows_exponentially(void) {
    unsigned long d0 = wifiBackoffMs(0);
    unsigned long d1 = wifiBackoffMs(1);
    unsigned long d2 = wifiBackoffMs(2);

    // Base terms dominate the jitter (each base doubles), so ordering is
    // guaranteed regardless of the jitter draw.
    TEST_ASSERT_TRUE_MESSAGE(d0 < d1, "attempt 1 must back off longer than attempt 0");
    TEST_ASSERT_TRUE_MESSAGE(d1 < d2, "attempt 2 must back off longer than attempt 1");
}

// ---------------------------------------------------------------------------
// Exact base + jitter bounds for low attempts
// ---------------------------------------------------------------------------

void test_backoff_low_attempt_bounds(void) {
    // base = 1000 << attempt; result in [base, base + base/4]
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(0) >= 1000, "attempt 0 base is 1000 ms");
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(0) <= 1250, "attempt 0 jitter stays under 25%");

    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(1) >= 2000, "attempt 1 base is 2000 ms");
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(1) <= 2500, "attempt 1 jitter stays under 25%");

    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(4) >= 16000, "attempt 4 base is 16000 ms");
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(4) <= 20000, "attempt 4 jitter stays under 25%");
}

// ---------------------------------------------------------------------------
// Cap at the max backoff — high attempts must not grow forever (E6: no
// infinite oscillation; combined with the fatal timeout this terminates).
// ---------------------------------------------------------------------------

void test_backoff_caps_at_max(void) {
    // attempt >= 5 caps the base at 30000 ms; +25% jitter = 37500 max.
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(5) >= 30000, "attempt 5 base capped at 30000 ms");
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(5) <= 37500, "attempt 5 stays under cap + jitter");

    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(20) >= 30000, "attempt 20 still capped at 30000 ms");
    TEST_ASSERT_TRUE_MESSAGE(wifiBackoffMs(20) <= 37500, "attempt 20 stays under cap + jitter");
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_wifiBackoff(void) {
    RUN_TEST(test_backoff_grows_exponentially);
    RUN_TEST(test_backoff_low_attempt_bounds);
    RUN_TEST(test_backoff_caps_at_max);
}
