#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class ScoreManager;   // forward declaration
class WiFiHandler;    // forward declaration (D4 — full type only in DisplayManager.cpp)

class DisplayManager {
public:
    /// OLED state machine states
    enum class State : uint8_t {
        BOOT,          ///< "RallyTap-01" centered, 2s auto-advance → CONNECTING
        IDLE,          ///< legacy "Conectando..." (superseded by CONNECTING)
        CONNECTING,    ///< waiting for the AP; "Wrong AP" after the fatal timeout (FW-1/E6)
        CONNECTED,     ///< "Mesa N ✓" + names + score (FW-2/FW-3)
        PRESSING,      ///< Flash inverted 100ms → CONNECTED
        CONFIRMING,    ///< "{score} OK" 1.5s → CONNECTED
        ERROR,         ///< "{score} X {msg}" until next state change
        RECONNECTING,  ///< "Mesa N ⚉ buscando hub" (FW-3)
        UNBOUND,       ///< pairing affordance "Pair me <call-sign>" (BND-5)
        FINISHED,      ///< "Fin: 3–2" + "new match from mobile" (MATCH-2)
        SESSION_END,   ///< "Sesion finalizada" + "pair from mobile" (REQ-FB-5)
        SLEEP          ///< display off after inactivity; wake on any activity (E13/E15)
    };

    /// Connected-state inactivity timeout before falling to SLEEP (E13).
    static constexpr unsigned long SLEEP_TIMEOUT_MS = 60000;
    /// SESSION_END dwell before auto-advancing to UNBOUND (REQ-FB-5).
    static constexpr unsigned long SESSION_END_TIMEOUT_MS = 4000;

    DisplayManager();

    /// Initialise I2C (SDA=21, SCL=22) and attempt SSD1306 detection.
    /// If no OLED is found, runs headless (_displayAvailable = false).
    void begin();

    /// Update display with data from the given ScoreManager.
    /// Keeps a reference for rendering CONNECTED state.
    void setScore(ScoreManager& score);

    /// Transition the state machine to a new state and re-render.
    void setState(State newState);

    /// Called every loop(); handles timed auto-advances and idle→SLEEP.
    void tick();

    /// Wake the display from SLEEP (button press or incoming frame, E13/E15).
    /// No-op when not sleeping.
    void wake();

    /// Mesa identity (mesaId = court-N from the downlink) for "Mesa N" renders.
    void setMesaId(const String& mesaId) { _mesaId = mesaId; }

    /// Real court display name from the downlink (e.g. "Mesa 1"); empty when absent.
    void setCourtName(const String& courtName) { _courtName = courtName; }

    /// 4-char call-sign for the unbound pairing affordance (BND-5).
    void setCallSign(const String& callSign) { _callSign = callSign; }

    /// Mark the wrong-AP fatal condition (rendered by CONNECTING, E6).
    void setWrongAp(bool wrongAp) { _wrongAp = wrongAp; }

    /// Track whether the bound court has an active match (REQ-FB-3).
    /// CONNECTED renders truncated names + score when true, the no-match
    /// hint when false; re-renders immediately when already CONNECTED.
    void setMatchActive(bool active) {
        _matchActive = active;
        if (_state == State::CONNECTED) render();
    }

    /// Inject the WiFi handler so CONNECTING can probe the AP link (D4).
    void setWifiHandler(WiFiHandler* wifi) { _wifi = wifi; }

    /// Current state (main uses it to decide wake targets).
    State state() const { return _state; }

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

    // Context for connected / unbound / wrong-AP renders
    String _mesaId;
    String _courtName;
    String _callSign;
    bool   _wrongAp;

    // Match-activity flag (REQ-FB-3) + CONNECTING phase-line probe (D4)
    bool         _matchActive;
    WiFiHandler* _wifi;
    bool         _apUpLast;

    // --- helpers ---
    void transitionTo(State s);
    void render();
    void cacheCurrentScore();
    String mesaNumber() const;
    String mesaLabel() const;

    // Per-state renderers
    void renderBoot();
    void renderIdle();
    void renderConnecting();
    void renderConnected();
    void renderPressing();
    void renderConfirming();
    void renderError();
    void renderReconnecting();
    void renderUnbound();
    void renderFinished();
    void renderSessionEnd();
    void renderSleep();
};

#endif // DISPLAY_MANAGER_H
