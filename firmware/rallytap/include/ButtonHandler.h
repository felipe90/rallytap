#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

/// Result of a single poll() call.
/// NONE means no valid press detected.
enum class PressResult : uint8_t {
    NONE = 0,
    A,
    B
};

class ButtonHandler {
public:
    /// Bounded FIFO capacity (FW-4); overflow drops the oldest (E12).
    static constexpr size_t FIFO_CAPACITY = 32;

    ButtonHandler();

    /// Configure GPIO pins for Button A and Button B.
    /// Both pins are set to INPUT_PULLUP (active LOW).
    void begin(int pinA, int pinB);

    /// Stable tap identity used in the uplink JSON.
    void setDevId(const String& devId) { _devId = devId; }
    const String& devId() const { return _devId; }

    /// Must be called every loop() iteration.
    /// Returns A or B when a debounced press is detected,
    /// NONE otherwise.
    PressResult poll();

    /// Build the tap->hub score uplink `{type, devId, button}` (FW-4).
    /// Carries NO mesaId/ts (A7/PROTO-2) — pure, host-testable.
    static String buttonJson(const String& devId, PressResult button);

    /// Enqueue a press event into the bounded FIFO. When full, drops the
    /// OLDEST event and logs a WARN, preserving the newest (E12).
    void enqueue(PressResult button);

    /// Number of buffered events.
    size_t pendingCount() const { return _count; }

    /// Remove and return the next buffered event in FIFO order (E8).
    /// Returns false when the FIFO is empty.
    bool dequeue(String& out);

    /// Drop all buffered events (e.g. press during SLEEP with no match, E15).
    void clear();

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

    // Bounded press FIFO (FW-4)
    String _devId;
    String _fifo[FIFO_CAPACITY];
    size_t _head;
    size_t _tail;
    size_t _count;
};

#endif // BUTTON_HANDLER_H
