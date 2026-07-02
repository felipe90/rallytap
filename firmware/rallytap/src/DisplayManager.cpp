#include "DisplayManager.h"
#include "ScoreManager.h"

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
{}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void DisplayManager::begin() {
    // Initialise I2C with the correct pins
    Wire.begin(21, 22);   // SDA = GPIO21, SCL = GPIO22

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
    _state       = State::BOOT;
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
                transitionTo(State::IDLE);
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

        default:
            // IDLE, CONNECTED, ERROR, RECONNECTING — no auto-advance
            break;
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
        case State::CONNECTED:     renderConnected();     break;
        case State::PRESSING:      renderPressing();      break;
        case State::CONFIRMING:    renderConfirming();    break;
        case State::ERROR:         renderError();         break;
        case State::RECONNECTING:  renderReconnecting();  break;
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

void DisplayManager::renderConnected() {
    _display.clearDisplay();

    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);

    if (_score != nullptr) {
        _display.setCursor(0, 12);
        _display.println(_score->formatDisplay());

        _display.setTextSize(1);
        _display.setCursor(0, 48);
        _display.println(" Connected");
    } else {
        _display.setCursor(0, 24);
        _display.println("Connected");
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
    _display.setCursor(0, 28);
    _display.println(" Reconnecting...");

    _display.display();
}
