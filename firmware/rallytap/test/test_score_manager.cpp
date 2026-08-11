/**
 * Unit tests for ScoreManager
 *
 * Built with PlatformIO's built-in Unity test framework.
 * Compiled via [env:native] (host machine, not ESP32).
 */
#include <Arduino.h>
#include <unity.h>
#include "ScoreManager.h"

// Include the implementation directly so it compiles for native (host) tests.
// PlatformIO's native env does not build src/ files by default.
#include "../src/ScoreManager.cpp"

// ---------------------------------------------------------------------------
// Fixture helpers
// ---------------------------------------------------------------------------

static ScoreManager sm;

void runTests_score(void);   // registered by test_main.cpp

// Unity's setUp/tearDown live in test_main.cpp; reset here per test.
static void resetScore() { sm = ScoreManager(); }

// ---------------------------------------------------------------------------
// fromJSON() — valid payload
// ---------------------------------------------------------------------------

void test_fromJSON_valid_score(void) {
    const char* json = "{\"a\":3,\"b\":2,\"set_a\":1,\"set_b\":0}";

    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(json), "Valid JSON should return true");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, sm.getA(),     "a should be 3");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, sm.getB(),     "b should be 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sm.getSetA(),  "set_a should be 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, sm.getSetB(),  "set_b should be 0");
    TEST_ASSERT_EQUAL_STRING("ok", sm.getStatus().c_str());
    TEST_ASSERT_EQUAL_STRING("",   sm.getMsg().c_str());
}

// ---------------------------------------------------------------------------
// fromJSON() — malformed JSON (returns false)
// ---------------------------------------------------------------------------

void test_fromJSON_malformed_returns_false(void) {
    const char* json = "this is not json";

    TEST_ASSERT_FALSE_MESSAGE(sm.fromJSON(json),
                              "Malformed JSON should return false");
}

// ---------------------------------------------------------------------------
// fromJSON() — empty string (returns false)
// ---------------------------------------------------------------------------

void test_fromJSON_empty_returns_false(void) {
    const char* json = "";

    TEST_ASSERT_FALSE_MESSAGE(sm.fromJSON(json),
                              "Empty string should return false");
}

// ---------------------------------------------------------------------------
// fromJSON() — partial fields → defaults applied
// ---------------------------------------------------------------------------

void test_fromJSON_partial_fields_default_values(void) {
    const char* json = "{\"a\":5}";

    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(json), "Partial JSON should return true");

    TEST_ASSERT_EQUAL_INT_MESSAGE(5,   sm.getA(),     "a should be 5");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0,   sm.getB(),     "b defaults to 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0,   sm.getSetA(),  "set_a defaults to 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0,   sm.getSetB(),  "set_b defaults to 0");
    TEST_ASSERT_EQUAL_STRING("ok", sm.getStatus().c_str());
    TEST_ASSERT_EQUAL_STRING("",   sm.getMsg().c_str());
}

// ---------------------------------------------------------------------------
// fromJSON() — extra fields are silently ignored
// ---------------------------------------------------------------------------

void test_fromJSON_extra_fields_ignored(void) {
    const char* json = "{"
        "\"a\":10,\"b\":7,"
        "\"set_a\":2,\"set_b\":1,"
        "\"extra1\":\"should be ignored\","
        "\"extra2\":42"
        "}";

    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(json),
                             "JSON with extra fields should return true");

    TEST_ASSERT_EQUAL_INT_MESSAGE(10,  sm.getA(),    "a should be 10");
    TEST_ASSERT_EQUAL_INT_MESSAGE(7,   sm.getB(),    "b should be 7");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2,   sm.getSetA(), "set_a should be 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1,   sm.getSetB(), "set_b should be 1");
    TEST_ASSERT_EQUAL_STRING("ok", sm.getStatus().c_str());
    TEST_ASSERT_EQUAL_STRING("",   sm.getMsg().c_str());
}

// ---------------------------------------------------------------------------
// fromJSON() — status field set to "error"
// ---------------------------------------------------------------------------

void test_fromJSON_status_error(void) {
    const char* json = "{\"status\":\"error\"}";

    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(json),
                             "JSON with status=error should return true");

    TEST_ASSERT_EQUAL_STRING("error", sm.getStatus().c_str());
}

// ---------------------------------------------------------------------------
// fromJSON() — msg field
// ---------------------------------------------------------------------------

void test_fromJSON_msg_field(void) {
    const char* json = "{\"msg\":\"Player A wins!\"}";

    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(json),
                             "JSON with msg field should return true");

    TEST_ASSERT_EQUAL_STRING("Player A wins!", sm.getMsg().c_str());
}

// ---------------------------------------------------------------------------
// formatDisplay()
// ---------------------------------------------------------------------------

void test_formatDisplay_output(void) {
    sm.fromJSON("{\"a\":3,\"b\":2}");

    String display = sm.formatDisplay();

    TEST_ASSERT_EQUAL_STRING("A:3 B:2", display.c_str());
}

// ---------------------------------------------------------------------------
// Getters — return correct values after parsing
// ---------------------------------------------------------------------------

void test_getters_after_parsing(void) {
    const char* json = "{"
        "\"a\":7,\"b\":5,"
        "\"set_a\":2,\"set_b\":1,"
        "\"status\":\"ok\","
        "\"msg\":\"test message\""
        "}";

    sm.fromJSON(json);

    TEST_ASSERT_EQUAL_INT_MESSAGE(7,             sm.getA(),     "a should be 7");
    TEST_ASSERT_EQUAL_INT_MESSAGE(5,             sm.getB(),     "b should be 5");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2,             sm.getSetA(),  "set_a should be 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1,             sm.getSetB(),  "set_b should be 1");
    TEST_ASSERT_EQUAL_STRING("ok",             sm.getStatus().c_str());
    TEST_ASSERT_EQUAL_STRING("test message",   sm.getMsg().c_str());
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_score(void) {
    resetScore(); RUN_TEST(test_fromJSON_valid_score);
    resetScore(); RUN_TEST(test_fromJSON_malformed_returns_false);
    resetScore(); RUN_TEST(test_fromJSON_empty_returns_false);
    resetScore(); RUN_TEST(test_fromJSON_partial_fields_default_values);
    resetScore(); RUN_TEST(test_fromJSON_extra_fields_ignored);
    resetScore(); RUN_TEST(test_fromJSON_status_error);
    resetScore(); RUN_TEST(test_fromJSON_msg_field);
    resetScore(); RUN_TEST(test_formatDisplay_output);
    resetScore(); RUN_TEST(test_getters_after_parsing);
}
