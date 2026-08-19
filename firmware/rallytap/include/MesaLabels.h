#ifndef MESA_LABELS_H
#define MESA_LABELS_H

#include <Arduino.h>

/// Pure mesa-label helpers — host-testable (native env) and used by
/// DisplayManager for the "Mesa N" / court-name OLED status line.
///
/// Precedence (2726faa): a real court display name from the downlink wins;
/// otherwise derive "Mesa N" from the trailing digits of the mesaId
/// ("court-3" -> "Mesa 3"); fall back to the bare "Mesa" when no digit
/// sequence exists.
///
/// The DOWNLINK carries `courtName` (e.g. "Mesa 1") so the OLED can show
/// the REAL court name, not a UUID-derived number.

/// "court-3" -> "3". Falls back to the raw mesaId when there is no trailing
/// digit sequence (e.g. empty or a label without a number).
static inline String mesaNumberFor(const String& mesaId) {
    const size_t len = mesaId.length();
    for (size_t i = len; i > 0; i--) {
        char c = mesaId[i - 1];
        if (c >= '0' && c <= '9') continue;
        if (i == len) return "";                 // no trailing digits
        return mesaId.substring(i, len);         // digits run [i, len)
    }
    return mesaId;
}

/// courtName > "Mesa N" > "Mesa" (2726faa courtName precedence).
static inline String mesaLabelFor(const String& courtName, const String& mesaId) {
    if (courtName.length() > 0) return courtName;
    String n = mesaNumberFor(mesaId);
    return n.length() > 0 ? "Mesa " + n : "Mesa";
}

#endif // MESA_LABELS_H
