/**
 * Unit tests for the mesa label precedence (2726faa — courtName from the
 * DOWNLINK wins over the derived "Mesa N", which wins over the bare "Mesa").
 *
 * The label logic is a pure function in MesaLabels.h so it can run on the
 * host via [env:native] without the SSD1306/Wire hardware seam. DisplayManager
 * delegates its private mesaLabel()/mesaNumber() to these helpers, so these
 * tests pin the exact OLED status-line text production uses.
 */
#include <Arduino.h>
#include <unity.h>
#include "../include/MesaLabels.h"

void runTests_mesaLabels(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// courtName precedence (FW-3 fix: real court name, not the UUID-derived one)
// ---------------------------------------------------------------------------

void test_mesaLabel_prefers_courtName(void) {
    // "court-3" would derive "Mesa 3", but the DOWNLINK name wins.
    TEST_ASSERT_EQUAL_STRING("Mesa 1", mesaLabelFor("Mesa 1", "court-3").c_str());
    TEST_ASSERT_EQUAL_STRING("Mesa 10", mesaLabelFor("Mesa 10", "court-3").c_str());
}

// ---------------------------------------------------------------------------
// Derived "Mesa N" when no courtName arrives
// ---------------------------------------------------------------------------

void test_mesaLabel_derives_from_mesaId(void) {
    TEST_ASSERT_EQUAL_STRING("Mesa 3", mesaLabelFor("", "court-3").c_str());
    TEST_ASSERT_EQUAL_STRING("Mesa 12", mesaLabelFor("", "court-12").c_str());
    TEST_ASSERT_EQUAL_STRING("Mesa 0", mesaLabelFor("", "court-0").c_str());
}

// ---------------------------------------------------------------------------
// Bare "Mesa" fallback when there is no trailing digit sequence
// ---------------------------------------------------------------------------

void test_mesaLabel_falls_back_to_bare_mesa(void) {
    TEST_ASSERT_EQUAL_STRING("Mesa", mesaLabelFor("", "court").c_str());
    TEST_ASSERT_EQUAL_STRING("Mesa", mesaLabelFor("", "").c_str());
}

// ---------------------------------------------------------------------------
// mesaNumberFor trailing-digit extraction
// ---------------------------------------------------------------------------

void test_mesaNumberFor_extracts_trailing_digits(void) {
    TEST_ASSERT_EQUAL_STRING("3", mesaNumberFor("court-3").c_str());
    TEST_ASSERT_EQUAL_STRING("12", mesaNumberFor("court-12").c_str());
    TEST_ASSERT_EQUAL_STRING("0", mesaNumberFor("court-0").c_str());
}

void test_mesaNumberFor_handles_no_digits(void) {
    TEST_ASSERT_EQUAL_STRING("", mesaNumberFor("court").c_str());
    TEST_ASSERT_EQUAL_STRING("", mesaNumberFor("").c_str());
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_mesaLabels(void) {
    RUN_TEST(test_mesaLabel_prefers_courtName);
    RUN_TEST(test_mesaLabel_derives_from_mesaId);
    RUN_TEST(test_mesaLabel_falls_back_to_bare_mesa);
    RUN_TEST(test_mesaNumberFor_extracts_trailing_digits);
    RUN_TEST(test_mesaNumberFor_handles_no_digits);
}
