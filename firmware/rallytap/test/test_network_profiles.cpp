/**
 * Unit tests for the multi-AP profile rotation decisions (d7a2b5c).
 *
 * WiFiHandler rotates through its configured AP profiles (deploy AP -> dev AP)
 * so a tap can fall back to the dev network when the deploy AP is down. The
 * pure decision logic (when to rotate, how the profile index wraps) lives in
 * NetworkProfiles.h so it can run on the host via [env:native] without the
 * WiFi.h / WebSocketsClient.h hardware seam.
 */
#include <Arduino.h>
#include <unity.h>
#include "../include/NetworkProfiles.h"

void runTests_networkProfiles(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// Rotation threshold — each profile gets PROFILE_ATTEMPTS_PER_PROFILE ticks
// ---------------------------------------------------------------------------

void test_shouldRotate_past_attempt_budget(void) {
    TEST_ASSERT_FALSE_MESSAGE(wifiShouldRotateProfile(0, 4), "first tick must stay on the profile");
    TEST_ASSERT_FALSE_MESSAGE(wifiShouldRotateProfile(3, 4), "last allowed tick must stay on the profile");
    TEST_ASSERT_TRUE_MESSAGE(wifiShouldRotateProfile(4, 4), "tick after the budget must rotate");
    TEST_ASSERT_TRUE_MESSAGE(wifiShouldRotateProfile(9, 4), "far past budget must rotate");
}

// ---------------------------------------------------------------------------
// Profile-index wrap — cycles and never runs off the end
// ---------------------------------------------------------------------------

void test_nextProfileIndex_wraps(void) {
    TEST_ASSERT_EQUAL_UINT32(1, wifiNextProfileIndex(0, 2));
    TEST_ASSERT_EQUAL_UINT32(0, wifiNextProfileIndex(1, 2));
    // From the last profile the wrap must go back to the first.
    TEST_ASSERT_EQUAL_UINT32(0, wifiNextProfileIndex(1, 2));
}

void test_nextProfileIndex_single_profile_stays(void) {
    // begin() coerces profileCount to >= 1, so a single AP never wraps.
    TEST_ASSERT_EQUAL_UINT32(0, wifiNextProfileIndex(0, 1));
}

void test_nextProfileIndex_zero_profile_count_degrades(void) {
    // Defensive: count==0 would modulo-by-zero; helper degrades to index 0.
    TEST_ASSERT_EQUAL_UINT32(0, wifiNextProfileIndex(0, 0));
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_networkProfiles(void) {
    RUN_TEST(test_shouldRotate_past_attempt_budget);
    RUN_TEST(test_nextProfileIndex_wraps);
    RUN_TEST(test_nextProfileIndex_single_profile_stays);
    RUN_TEST(test_nextProfileIndex_zero_profile_count_degrades);
}
