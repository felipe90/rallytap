/**
 * Unit tests for the ButtonHandler bounded press FIFO (FW-4 / A7 / E8 / E12).
 *
 * The FIFO buffers tap->hub score uplinks during an outage and flushes them on
 * reconnect in arrival order. Events carry only {type, devId, button} — never
 * mesaId or ts (A7/PROTO-2). Overflow (32) drops the OLDEST event with a WARN
 * and preserves the newest (E12).
 */
#include <Arduino.h>
#include <unity.h>
#include "ButtonHandler.h"

#include "../src/ButtonHandler.cpp"

static ButtonHandler bh;

void runTests_buttonHandler(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// buttonJson() — exact uplink shape (A7: no mesaId/ts)
// ---------------------------------------------------------------------------

void test_buttonJson_shapes_A(void) {
    String json = ButtonHandler::buttonJson("dev-tap-TEST", PressResult::A);

    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"A\"}",
        json.c_str());
    TEST_ASSERT_NULL_MESSAGE(strstr(json.c_str(), "mesaId"),
                             "uplink must not carry mesaId (A7)");
}

void test_buttonJson_shapes_B(void) {
    String json = ButtonHandler::buttonJson("dev-tap-TEST", PressResult::B);

    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"B\"}",
        json.c_str());
}

// ---------------------------------------------------------------------------
// FIFO order (E8)
// ---------------------------------------------------------------------------

void test_fifo_order_preserved(void) {
    bh.setDevId("dev-tap-TEST");
    bh.clear();

    bh.enqueue(PressResult::A);
    bh.enqueue(PressResult::B);
    bh.enqueue(PressResult::A);

    TEST_ASSERT_EQUAL_UINT(3, bh.pendingCount());

    String evt;
    TEST_ASSERT_TRUE(bh.dequeue(evt));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"A\"}",
        evt.c_str());
    TEST_ASSERT_TRUE(bh.dequeue(evt));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"B\"}",
        evt.c_str());
    TEST_ASSERT_TRUE(bh.dequeue(evt));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"A\"}",
        evt.c_str());

    TEST_ASSERT_EQUAL_UINT(0, bh.pendingCount());
    TEST_ASSERT_FALSE_MESSAGE(bh.dequeue(evt), "empty FIFO must not dequeue");
}

// ---------------------------------------------------------------------------
// Overflow drops the oldest, keeps the newest (E12)
// ---------------------------------------------------------------------------

void test_fifo_overflow_drops_oldest(void) {
    bh.setDevId("dev-tap-TEST");
    bh.clear();

    const size_t N = ButtonHandler::FIFO_CAPACITY + 2;   // 34 presses
    for (size_t i = 0; i < N; i++) {
        bh.enqueue((i % 2 == 0) ? PressResult::A : PressResult::B);
    }

    TEST_ASSERT_EQUAL_UINT(ButtonHandler::FIFO_CAPACITY, bh.pendingCount());

    // Events 1 and 2 (the oldest) must be gone; the FIFO holds 3..34.
    String evt;
    TEST_ASSERT_TRUE(bh.dequeue(evt));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"A\"}",
        evt.c_str());

    while (bh.dequeue(evt)) {
        // drain; last one is the newest press (#34, index 33 -> button B)
    }
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"rallytap.score\",\"devId\":\"dev-tap-TEST\",\"button\":\"B\"}",
        evt.c_str());
}

// ---------------------------------------------------------------------------
// clear() drops everything
// ---------------------------------------------------------------------------

void test_fifo_clear(void) {
    bh.setDevId("dev-tap-TEST");
    bh.clear();

    bh.enqueue(PressResult::A);
    bh.enqueue(PressResult::B);
    bh.clear();

    TEST_ASSERT_EQUAL_UINT(0, bh.pendingCount());

    String evt;
    TEST_ASSERT_FALSE_MESSAGE(bh.dequeue(evt), "cleared FIFO must be empty");
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_buttonHandler(void) {
    RUN_TEST(test_buttonJson_shapes_A);
    RUN_TEST(test_buttonJson_shapes_B);
    RUN_TEST(test_fifo_order_preserved);
    RUN_TEST(test_fifo_overflow_drops_oldest);
    RUN_TEST(test_fifo_clear);
}
