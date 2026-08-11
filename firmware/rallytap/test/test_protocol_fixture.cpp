/**
 * RallyTap protocol fixture — firmware mirror of the M1 coordinated-bump gate.
 *
 * The hub repo pins these envelope shapes + PROTOCOL_VERSION in
 * shared/__tests__/rallytap-protocol.test.ts. This native test parses the SAME
 * fixture JSON so a protocol-version bump MUST land in both repos in one change
 * (PROTO-5 / M1). ScoreManager parses the envelope payloads exactly as the
 * firmware main.cpp dispatches them.
 */
#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "ScoreManager.h"

// ScoreManager definitions are linked from test_score_manager.cpp (single
// [env:native] binary); this file only exercises the public API + the fixture
// envelope shapes.

/// Mirror of `shared/rallyTap.ts` PROTOCOL_VERSION (M1 gate).
static const int PROTOCOL_VERSION = 1;

// Exact fixture JSON shared with the hub's shared/__tests__ fixture.
static const char* FIXTURE_BIND =
    "{"
    "\"type\":\"rallytap.bind\","
    "\"mesaId\":\"court-3\","
    "\"paired\":true,"
    "\"match\":{\"matchId\":\"M-42\",\"status\":\"LIVE\",\"winner\":null,"
    "\"score_a\":3,\"score_b\":2,\"set_a\":2,\"set_b\":1},"
    "\"leftName\":\"Pedro\",\"rightName\":\"Juan\","
    "\"score\":{\"a\":3,\"b\":2,\"set_a\":2,\"set_b\":1,\"status\":\"LIVE\",\"msg\":\"\"}"
    "}";

static const char* FIXTURE_REGISTER =
    "{\"type\":\"rallytap.register\",\"devId\":\"dev-tap-AC12\",\"label\":\"7B3D\",\"fw\":\"2.0.0\"}";

static const char* FIXTURE_BUTTON =
    "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-AC12\",\"button\":\"A\"}";

static const char* FIXTURE_MATCH_NULL =
    "{"
    "\"type\":\"rallytap.match\","
    "\"mesaId\":\"court-3\","
    "\"match\":null,\"leftName\":null,\"rightName\":null,"
    "\"score\":{\"a\":0,\"b\":0,\"set_a\":0,\"set_b\":0,\"status\":\"FINISHED\",\"msg\":\"\"}"
    "}";

static const char* FIXTURE_SCORE_DOWNLINK =
    "{"
    "\"type\":\"rallytap.score\","
    "\"mesaId\":\"court-3\","
    "\"score\":{\"a\":3,\"b\":2,\"set_a\":2,\"set_b\":1,\"status\":\"LIVE\",\"msg\":\"\"},"
    "\"leftName\":\"Pedro\",\"rightName\":\"Juan\""
    "}";

void runTests_protocolFixture(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// PROTOCOL_VERSION pin (M1)
// ---------------------------------------------------------------------------

void test_protocol_version_is_1(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, PROTOCOL_VERSION,
                                  "bump both repo mirrors together (M1)");
}

// ---------------------------------------------------------------------------
// register envelope (uplink)
// ---------------------------------------------------------------------------

void test_fixture_register_shape(void) {
    StaticJsonDocument<256> doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, FIXTURE_REGISTER).code(),
                              "register fixture must parse");

    TEST_ASSERT_EQUAL_STRING("rallytap.register", doc["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("dev-tap-AC12",      doc["devId"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("7B3D",              doc["label"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("2.0.0",             doc["fw"].as<const char*>());
}

// ---------------------------------------------------------------------------
// button envelope (uplink) — no mesaId/ts (A7/PROTO-2)
// ---------------------------------------------------------------------------

void test_fixture_button_shape_no_mesaId_ts(void) {
    StaticJsonDocument<128> doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, FIXTURE_BUTTON).code(),
                              "button fixture must parse");

    TEST_ASSERT_EQUAL_STRING("rallytap.score", doc["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("dev-tap-AC12",   doc["devId"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("A",              doc["button"].as<const char*>());
    TEST_ASSERT_TRUE_MESSAGE(doc["mesaId"].isNull(), "uplink must not carry mesaId (A7)");
    TEST_ASSERT_TRUE_MESSAGE(doc["ts"].isNull(),     "uplink must not carry ts (A7)");
}

// ---------------------------------------------------------------------------
// bind envelope (downlink) — full shape
// ---------------------------------------------------------------------------

void test_fixture_bind_shape(void) {
    StaticJsonDocument<512> doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, FIXTURE_BIND).code(),
                              "bind fixture must parse");

    TEST_ASSERT_EQUAL_STRING("rallytap.bind", doc["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("court-3",       doc["mesaId"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(1,                  doc["paired"].as<int>());
    TEST_ASSERT_EQUAL_STRING("M-42",          doc["match"]["matchId"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("LIVE",          doc["match"]["status"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Pedro",         doc["leftName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Juan",          doc["rightName"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(3,                  doc["score"]["a"].as<int>());
    TEST_ASSERT_EQUAL_INT(2,                  doc["score"]["b"].as<int>());
    TEST_ASSERT_EQUAL_INT(2,                  doc["score"]["set_a"].as<int>());
    TEST_ASSERT_EQUAL_INT(1,                  doc["score"]["set_b"].as<int>());
    TEST_ASSERT_EQUAL_STRING("LIVE",          doc["score"]["status"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("",              doc["score"]["msg"].as<const char*>());
}

void test_fixture_bind_through_score_manager(void) {
    ScoreManager sm;
    TEST_ASSERT_TRUE_MESSAGE(sm.fromJSON(FIXTURE_BIND),
                             "bind payload must parse via ScoreManager");

    TEST_ASSERT_EQUAL_STRING("Pedro", sm.getLeftName().c_str());
    TEST_ASSERT_EQUAL_STRING("Juan",  sm.getRightName().c_str());
    TEST_ASSERT_EQUAL_STRING("Pedro 3\nJuan 2", sm.formatDisplay().c_str());
    TEST_ASSERT_EQUAL_INT(3, sm.getA());
    TEST_ASSERT_EQUAL_INT(2, sm.getB());
    TEST_ASSERT_EQUAL_INT(2, sm.getSetA());
    TEST_ASSERT_EQUAL_INT(1, sm.getSetB());
}

// ---------------------------------------------------------------------------
// match:null envelope (match end, MATCH-2)
// ---------------------------------------------------------------------------

void test_fixture_match_null_shape(void) {
    StaticJsonDocument<512> doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, FIXTURE_MATCH_NULL).code(),
                              "match:null fixture must parse");

    TEST_ASSERT_EQUAL_STRING("rallytap.match", doc["type"].as<const char*>());
    TEST_ASSERT_TRUE_MESSAGE(doc["match"].isNull(), "match must be null at end");
    TEST_ASSERT_EQUAL_STRING("FINISHED", doc["score"]["status"].as<const char*>());
    TEST_ASSERT_TRUE_MESSAGE(doc["leftName"].isNull(), "names null with no match");
}

// ---------------------------------------------------------------------------
// score downlink (live push, MATCH-1)
// ---------------------------------------------------------------------------

void test_fixture_score_downlink_shape(void) {
    StaticJsonDocument<512> doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, FIXTURE_SCORE_DOWNLINK).code(),
                              "score downlink fixture must parse");

    TEST_ASSERT_EQUAL_STRING("rallytap.score", doc["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("court-3",        doc["mesaId"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(3,                   doc["score"]["a"].as<int>());
    TEST_ASSERT_EQUAL_STRING("Pedro",          doc["leftName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Juan",           doc["rightName"].as<const char*>());
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_protocolFixture(void) {
    RUN_TEST(test_protocol_version_is_1);
    RUN_TEST(test_fixture_register_shape);
    RUN_TEST(test_fixture_button_shape_no_mesaId_ts);
    RUN_TEST(test_fixture_bind_shape);
    RUN_TEST(test_fixture_bind_through_score_manager);
    RUN_TEST(test_fixture_match_null_shape);
    RUN_TEST(test_fixture_score_downlink_shape);
}
