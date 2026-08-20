#ifndef OLED_TEXT_H
#define OLED_TEXT_H

#include <Arduino.h>

/// Max characters that fit on one size-1 line of the 128x64 SSD1306
/// (6 px per char → 128 / 6 ≈ 21). Size-2 lines fit ~10 chars (12 px/char).
static constexpr uint8_t OLED_LINE_MAX = 21;

/// Fit arbitrary text on one OLED line (REQ-FB-2).
///
/// Keeps only printable ASCII 0x20..0x7E (the GFX font has no accents or
/// symbols — non-ASCII bytes are stripped, e.g. "Álvaro" → "lvaro"). Input at
/// or below `maxLen` is returned unchanged; longer input is truncated to
/// (maxLen − 3) chars + "...". Empty / all-non-ASCII input yields "".
///
/// Portable across the real Arduino String and the native-test stub: built via
/// `out = out + in.substring(i, i + 1)` (no `operator+=(char)` in the stub).
static inline String truncateForOled(const String& in, uint8_t maxLen = OLED_LINE_MAX) {
    if (maxLen < 3) {
        maxLen = 3;   // keep (maxLen - 3) from underflowing
    }

    // Filter to printable ASCII first.
    String out;
    for (unsigned int i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c >= 0x20 && c <= 0x7E) {
            out = out + in.substring(i, i + 1);
        }
    }

    if (out.length() <= maxLen) {
        return out;
    }
    return out.substring(0, maxLen - 3) + "...";
}

#endif // OLED_TEXT_H