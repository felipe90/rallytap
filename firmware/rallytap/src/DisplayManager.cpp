#include "DisplayManager.h"
#include "ScoreManager.h"
#include "MesaLabels.h"

// I2C pins for the OLED — board-configurable via build flags.
// Defaults match the RallyTap-01 board (SDA=21, SCL=22).
#ifndef OLED_SDA
#define OLED_SDA 21
#endif
#ifndef OLED_SCL
#define OLED_SCL 22
#endif

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DisplayManager::DisplayManager()
    : _display(128, 64, &Wire, -1)   // I2C, no reset pin
    , _score(nullptr)
    , _state(State::BOOT)
    , _prevState(State::BOOT)
    , _stateStartMs(0)
    , _displayAvailable(false)
    , _cachedA(0)
    , _cachedB(0)
    , _cachedMsg("")
    , _mesaId("")
    , _courtName("")
    , _callSign("")
    , _wrongAp(false)
{}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void DisplayManager::begin() {
    // Initialise I2C with the correct pins
    Wire.begin(OLED_SDA, OLED_SCL);   // SDA/SCL per board config

    // Probe for the OLED at address 0x3C
    Wire.beginTransmission(0x3C);
    if (Wire.endTransmission() != 0) {
        Serial.println("[Display] OLED not found — running headless");
        _displayAvailable = false;
        return;
    }

    // Try initialising the SSD1306
    if (!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[Display] SSD1306 init failed — running headless");
        _displayAvailable = false;
        return;
    }

    _displayAvailable = true;
    _display.clearDisplay();
    _display.display();

    // Initial state will be set by the caller (main.cpp)
    // but default to BOOT rendering
    _state        = State::BOOT;
    _stateStartMs = millis();
    render();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DisplayManager::setScore(ScoreManager& score) {
    _score = &score;
}

void DisplayManager::setState(State newState) {
    // Capture score data at the moment of transition for
    // states that need it (CONFIRMING, ERROR)
    if (newState == State::CONFIRMING || newState == State::ERROR) {
        cacheCurrentScore();
    }

    transitionTo(newState);
}

void DisplayManager::tick() {
    if (!_displayAvailable) return;

    unsigned long elapsed = millis() - _stateStartMs;

    switch (_state) {
        case State::BOOT:
            if (elapsed >= 2000) {
                transitionTo(State::CONNECTING);
            }
            break;

        case State::PRESSING:
            if (elapsed >= 100) {
                // Restore normal display and go back to CONNECTED
                _display.invertDisplay(false);
                transitionTo(State::CONNECTED);
            }
            break;

        case State::CONFIRMING:
            if (elapsed >= 1500) {
                transitionTo(State::CONNECTED);
            }
            break;

        case State::CONNECTED:
        case State::RECONNECTING:
        case State::UNBOUND:
        case State::FINISHED:
            // Connected states fall to SLEEP after inactivity (E13).
            if (elapsed >= SLEEP_TIMEOUT_MS) {
                transitionTo(State::SLEEP);
            }
            break;

        default:
            // CONNECTING, IDLE, ERROR, SLEEP — no auto-advance.
            break;
    }
}

void DisplayManager::wake() {
    if (_state == State::SLEEP) {
        transitionTo(State::CONNECTED);
    }
}

// ===========================================================================
// Private helpers
// ===========================================================================

void DisplayManager::transitionTo(State s) {
    // If leaving PRESSING state, restore normal display
    // (prevents stuck invert when CONFIRMING enters during PRESSING's 100ms window)
    if (_state == State::PRESSING && s != State::PRESSING) {
        _display.invertDisplay(false);
    }

    // Physical display on/off for SLEEP (E13/E15).
    if (s == State::SLEEP) {
        _display.ssd1306_command(SSD1306_DISPLAYOFF);
    } else if (_state == State::SLEEP) {
        _display.ssd1306_command(SSD1306_DISPLAYON);
    }

    _state        = s;
    _stateStartMs = millis();
    if (_displayAvailable) {
        render();
    }
}

void DisplayManager::cacheCurrentScore() {
    if (_score != nullptr) {
        _cachedA = _score->getA();
        _cachedB = _score->getB();
        _cachedMsg = _score->getMsg();
    }
}

// ===========================================================================
// Render dispatcher
// ===========================================================================

void DisplayManager::render() {
    if (!_displayAvailable) return;

    switch (_state) {
        case State::BOOT:          renderBoot();          break;
        case State::IDLE:          renderIdle();          break;
        case State::CONNECTING:    renderConnecting();    break;
        case State::CONNECTED:     renderConnected();     break;
        case State::PRESSING:      renderPressing();      break;
        case State::CONFIRMING:    renderConfirming();    break;
        case State::ERROR:         renderError();         break;
        case State::RECONNECTING:  renderReconnecting();  break;
        case State::UNBOUND:       renderUnbound();       break;
        case State::FINISHED:      renderFinished();      break;
        case State::SLEEP:         renderSleep();         break;
    }
}

// ===========================================================================
// Per-state renderers
// ===========================================================================

void DisplayManager::renderBoot() {
    _display.clearDisplay();

    // "RallyTap-01" centred
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(8, 24);
    _display.println("RallyTap");
    _display.setCursor(32, 44);
    _display.println("-01");

    _display.display();
}

void DisplayManager::renderIdle() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 28);
    _display.println(" Connecting...");

    _display.display();
}

void DisplayManager::renderConnecting() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);

    if (_wrongAp) {
        // Fatal wrong/unreachable AP (FW-1/E6) — CONNECTING stops oscillating.
        _display.setCursor(0, 20);
        _display.println("Wrong AP");
        _display.setCursor(0, 36);
        _display.println("check network");
    } else {
        _display.setCursor(0, 28);
        _display.println("Conectando...");
    }

    _display.display();
}

void DisplayManager::renderConnected() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    _display.println(mesaLabel() + " OK");   // ASCII for ✓ (GFX font)

    if (_score != nullptr) {
        _display.setTextSize(2);
        _display.setCursor(0, 18);
        _display.println(_score->formatDisplay());
    }

    _display.display();
}

void DisplayManager::renderPressing() {
    // Flash the current display inverted for 100ms
    _display.invertDisplay(true);
    _display.display();
}

void DisplayManager::renderConfirming() {
    _display.clearDisplay();

    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 12);
    _display.print("A:"); _display.print(_cachedA);
    _display.print(" B:"); _display.print(_cachedB);

    _display.setTextSize(1);
    _display.setCursor(0, 44);
    _display.println(" Updated");

    _display.display();
}

void DisplayManager::renderError() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    _display.print("A:"); _display.print(_cachedA);
    _display.print(" B:"); _display.print(_cachedB);
    _display.println(" X");

    _display.setCursor(0, 28);
    // Truncate msg to ~20 chars so it fits on one line
    String msg = _cachedMsg.length() > 20
        ? _cachedMsg.substring(0, 20) + "..."
        : _cachedMsg;
    _display.println(msg);

    _display.display();
}

void DisplayManager::renderReconnecting() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 20);
    _display.println(mesaLabel());          // ASCII for ⚉ (GFX font)
    _display.setCursor(0, 36);
    _display.println("buscando hub");

    _display.display();
}

void DisplayManager::renderUnbound() {
    _display.clearDisplay();

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 12);
    _display.println("Sin emparajar");
    _display.setCursor(0, 28);
    _display.println("Pair me " + _callSign);          // BND-5 — 4-char call-sign

    _display.display();
}

void DisplayManager::renderFinished() {
    _display.clearDisplay();

    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 14);
    _display.print("Fin: ");
    if (_score != nullptr) {
        _display.print(_score->getSetA());
        _display.print("-");
        _display.print(_score->getSetB());
    }

    _display.setTextSize(1);
    _display.setCursor(0, 44);
    _display.println("new match from mobile");         // MATCH-2

    _display.display();
}

void DisplayManager::renderSleep() {
    // The panel itself was switched off in transitionTo(State::SLEEP);
    // just keep the frame cleared for a clean re-render on wake.
    _display.clearDisplay();
    _display.display();
}

// ===========================================================================
// Mesa label helpers
// ===========================================================================

String DisplayManager::mesaLabel() const {
    // Pure logic in MesaLabels.h (host-tested in test_mesa_label.cpp).
    return mesaLabelFor(_courtName, _mesaId);
}

String DisplayManager::mesaNumber() const {
    // Pure logic in MesaLabels.h (host-tested in test_mesa_label.cpp).
    return mesaNumberFor(_mesaId);
}
