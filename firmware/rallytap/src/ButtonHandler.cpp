#include "ButtonHandler.h"

ButtonHandler::ButtonHandler()
    : _pinA(4)
    , _pinB(15)
    , _lastRawA(HIGH)
    , _lastRawB(HIGH)
    , _lastStableA(HIGH)
    , _lastStableB(HIGH)
    , _lastDebounceTimeA(0)
    , _lastDebounceTimeB(0)
    , _lastPressTime(0)
    , _cooldownActive(false)
    , _devId("")
    , _head(0)
    , _tail(0)
    , _count(0)
{}

void ButtonHandler::begin(int pinA, int pinB) {
    _pinA = pinA;
    _pinB = pinB;

    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);

    // Seed stable state with initial readings
    _lastStableA = digitalRead(_pinA);
    _lastStableB = digitalRead(_pinB);
    _lastRawA    = _lastStableA;
    _lastRawB    = _lastStableB;

    _lastPressTime     = 0;
    _cooldownActive    = false;
}

PressResult ButtonHandler::poll() {
    unsigned long now = millis();

    // -------------------------------------------------------
    // 1. Debounce Button A
    // -------------------------------------------------------
    {
        int reading = digitalRead(_pinA);
        if (reading != _lastRawA) {
            _lastDebounceTimeA = now;
            _lastRawA = reading;
        }

        if ((now - _lastDebounceTimeA) > DEBOUNCE_MS) {
            if (reading != _lastStableA) {
                _lastStableA = reading;
            }
        }
    }

    // -------------------------------------------------------
    // 2. Debounce Button B
    // -------------------------------------------------------
    {
        int reading = digitalRead(_pinB);
        if (reading != _lastRawB) {
            _lastDebounceTimeB = now;
            _lastRawB = reading;
        }

        if ((now - _lastDebounceTimeB) > DEBOUNCE_MS) {
            if (reading != _lastStableB) {
                _lastStableB = reading;
            }
        }
    }

    // -------------------------------------------------------
    // 3. Cooldown management
    // -------------------------------------------------------
    if (_cooldownActive && (now - _lastPressTime) >= COOLDOWN_MS) {
        _cooldownActive = false;
    }

    // -------------------------------------------------------
    // 4. Press detection (only when cooldown is inactive)
    // -------------------------------------------------------
    if (!_cooldownActive) {
        // Check A first (active LOW — pressed when LOW)
        if (_lastStableA == LOW) {
            _lastPressTime  = now;
            _cooldownActive = true;
            return PressResult::A;
        }

        // Then B
        if (_lastStableB == LOW) {
            _lastPressTime  = now;
            _cooldownActive = true;
            return PressResult::B;
        }
    }

    return PressResult::NONE;
}

// ===========================================================================
// Press uplink FIFO (FW-4 / A7 / E8 / E12)
// ===========================================================================

String ButtonHandler::buttonJson(const String& devId, PressResult button) {
    StaticJsonDocument<128> doc;
    doc["type"]   = "rallytap.score";
    doc["devId"]  = devId;
    doc["button"] = (button == PressResult::A) ? "A" : "B";

    String out;
    serializeJson(doc, out);
    return out;
}

void ButtonHandler::enqueue(PressResult button) {
    String json = buttonJson(_devId, button);

    if (_count == FIFO_CAPACITY) {
        // Overflow — drop the OLDEST event, keep the newest (E12).
        Serial.println("[Button] FIFO full — dropping oldest event");
        _head = (_head + 1) % FIFO_CAPACITY;
        _count--;
    }

    _fifo[_tail] = json;
    _tail = (_tail + 1) % FIFO_CAPACITY;
    _count++;
}

bool ButtonHandler::dequeue(String& out) {
    if (_count == 0) {
        return false;
    }

    out  = _fifo[_head];
    _head = (_head + 1) % FIFO_CAPACITY;
    _count--;
    return true;
}

void ButtonHandler::clear() {
    _head  = 0;
    _tail  = 0;
    _count = 0;
}
