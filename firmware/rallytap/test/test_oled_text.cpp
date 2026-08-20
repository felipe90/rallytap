/**
 * Unit tests for the OLED line truncation helper (REQ-FB-2).
 *
 * truncateForOled is a pure function in OledText.h so it runs on the host via
 * [env:native] without the SSD1306/Wire hardware seam — same pattern as
 * test_mesa_label.cpp.
 */
#include <Arduino.h>
#include <unity.h>
#include "../include/OledText.h"

void runTests_oledText(void);   // registered by test_main.cpp

// ---------------------------------------------------------------------------
// Short inputs pass through unchanged (no suffix)
// ---------------------------------------------------------------------------

void test_truncate_short_unchanged(void) {
    TEST_ASSERT_EQUAL_STRING("Juan", truncateForOled("Juan").c_str());
}

void test_truncate_exact_max_unchanged(void) {
    // Exactly 21 chars (a..u) — unchanged, no suffix.
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopqrstu",
                             truncateForOled("abcdefghijklmnopqrstu").c_str());
}

// ---------------------------------------------------------------------------
// Long inputs → (maxLen-3) + "..." (21 total at the default max)
// ---------------------------------------------------------------------------

void test_truncate_over_max_adds_ellipsis(void) {
    // 22 chars (a..v) → 18 chars + "..." = 21 total.
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopqr...",
                             truncateForOled("abcdefghijklmnopqrstuv").c_str());
}

void test_truncate_long_name_ascii_only(void) {
    const String out = truncateForOled("Alexandru Constantin Popescu");
    TEST_ASSERT_TRUE(out.length() <= OLED_LINE_MAX);
    TEST_ASSERT_TRUE(out.length() >= 3);
    // Ends with "..."
    TEST_ASSERT_EQUAL_STRING("...",
                             out.substring(out.length() - 3, out.length()).c_str());
    // ASCII-only
    for (unsigned int i = 0; i < out.length(); i++) {
        TEST_ASSERT_TRUE(out[i] >= 0x20 && out[i] <= 0x7E);
    }
}

// ---------------------------------------------------------------------------
// Non-ASCII bytes are stripped (GFX font has no accents)
// ---------------------------------------------------------------------------

void test_truncate_strips_accents(void) {
    // "Álvaro" — Á is two non-ASCII UTF-8 bytes, stripped → "lvaro".
    TEST_ASSERT_EQUAL_STRING("lvaro", truncateForOled("\xC3\x81lvaro").c_str());
}

void test_truncate_strips_accents_mixed(void) {
    // "Juan Pérez" — é stripped → "Juan Prez".
    TEST_ASSERT_EQUAL_STRING("Juan Prez",
                             truncateForOled("Juan P\xC3\xA9rez").c_str());
}

void test_truncate_all_non_ascii_empty(void) {
    TEST_ASSERT_EQUAL_STRING("", truncateForOled("\xC3\x81\xC3\xA9\xC3\xAD").c_str());
}

void test_truncate_empty_empty(void) {
    TEST_ASSERT_EQUAL_STRING("", truncateForOled("").c_str());
}

// ---------------------------------------------------------------------------
// Custom maxLen
// ---------------------------------------------------------------------------

void test_truncate_custom_max_len(void) {
    // maxLen=20, 25 chars (a..y) → 17 chars + "..." = 20 total.
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopq...",
                             truncateForOled("abcdefghijklmnopqrstuvwxy", 20).c_str());
}

// ---------------------------------------------------------------------------
// Runner — registered from test_main.cpp
// ---------------------------------------------------------------------------

void runTests_oledText(void) {
    RUN_TEST(test_truncate_short_unchanged);
    RUN_TEST(test_truncate_exact_max_unchanged);
    RUN_TEST(test_truncate_over_max_adds_ellipsis);
    RUN_TEST(test_truncate_long_name_ascii_only);
    RUN_TEST(test_truncate_strips_accents);
    RUN_TEST(test_truncate_strips_accents_mixed);
    RUN_TEST(test_truncate_all_non_ascii_empty);
    RUN_TEST(test_truncate_empty_empty);
    RUN_TEST(test_truncate_custom_max_len);
}