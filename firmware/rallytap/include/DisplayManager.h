#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class ScoreManager;   // forward declaration

class DisplayManager {
public:
    /// OLED state machine states (7 states)
    enum class State : uint8_t {
        BOOT,          ///< "RallyTap-01" centered, 2s auto-advance → IDLE
        IDLE,          ///< "Conectando..."
        CONNECTED,     ///< "A:{a} B:{b}"
        PRESSING,      ///< Flash inverted 100ms → CONNECTED
        CONFIRMING,    ///< "{score} OK" 1.5s → CONNECTED
        ERROR,         ///< "{score} X {msg}" until next state change
        RECONNECTING   ///< "Reconectando..."
    };

    DisplayManager();

    /// Initialise I2C (SDA=21, SCL=22) and attempt SSD1306 detection.
    /// If no OLED is found, runs headless (_displayAvailable = false).
    void begin();

    /// Update display with data from the given ScoreManager.
    /// Keeps a reference for rendering CONNECTED state.
    void setScore(ScoreManager& score);

    /// Transition the state machine to a new state and re-render.
    void setState(State newState);

    /// Called every loop(); handles timed auto-advances
    /// (BOOT→IDLE, PRESSING→CONNECTED, CONFIRMING→CONNECTED).
    void tick();

    /// Whether an OLED was detected and is usable.
    bool isDisplayAvailable() const { return _displayAvailable; }

private:
    Adafruit_SSD1306 _display;
    ScoreManager*    _score;

    State  _state;
    State  _prevState;
    unsigned long _stateStartMs;

    bool   _displayAvailable;

    // Cached score for CONFIRMING / ERROR render
    int    _cachedA;
    int    _cachedB;
    String _cachedMsg;

    // --- helpers ---
    void transitionTo(State s);
    void render();
    void cacheCurrentScore();

    // Per-state renderers
    void renderBoot();
    void renderIdle();
    void renderConnected();
    void renderPressing();
    void renderConfirming();
    void renderError();
    void renderReconnecting();
};

#endif // DISPLAY_MANAGER_H
