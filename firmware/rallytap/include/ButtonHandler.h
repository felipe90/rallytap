#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

/// Result of a single poll() call.
/// NONE means no valid press detected.
enum class PressResult : uint8_t {
    NONE = 0,
    A,
    B
};

class ButtonHandler {
public:
    ButtonHandler();

    /// Configure GPIO pins for Button A and Button B.
    /// Both pins are set to INPUT_PULLUP (active LOW).
    void begin(int pinA, int pinB);

    /// Must be called every loop() iteration.
    /// Returns A or B when a debounced press is detected,
    /// NONE otherwise.
    PressResult poll();

    // --- timing constants (exposed for verification) ---
    static constexpr unsigned long DEBOUNCE_MS  = 50;
    static constexpr unsigned long COOLDOWN_MS  = 300;

private:
    int  _pinA;
    int  _pinB;

    // Raw readings (last sampled)
    int  _lastRawA;
    int  _lastRawB;

    // Debounced stable state
    int  _lastStableA;
    int  _lastStableB;

    // Debounce timers (per-button)
    unsigned long _lastDebounceTimeA;
    unsigned long _lastDebounceTimeB;

    // Cooldown state
    unsigned long _lastPressTime;
    bool          _cooldownActive;
};

#endif // BUTTON_HANDLER_H
