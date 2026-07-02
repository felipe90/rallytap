/**
 * RallyTap Phase 1 — Firmware Orchestrator
 *
 * ESP32-WROOM-32 / Arduino Framework
 *
 * Data flow (one loop iteration):
 *   1. Poll buttons → on press: BLE notify + display PRESSING
 *   2. Check BLE connection state → update display (CONNECTED / RECONNECTING)
 *   3. Process pending score writes → update ScoreManager + display state
 *   4. Tick the display state machine (timed auto-advances)
 */

#include <Arduino.h>

#include "ScoreManager.h"
#include "ButtonHandler.h"
#include "BLEHandler.h"
#include "DisplayManager.h"

// ===========================================================================
// Global instances
// ===========================================================================

ScoreManager    scoreManager;
ButtonHandler   buttonHandler;
BLEHandler      bleHandler;
DisplayManager  displayManager;

// ===========================================================================
// BLE score-write bridge
//
// The BLE callback runs in a FreeRTOS task context (not the main loop).
// We use a volatile flag + a small buffer to safely hand data to loop().
// ===========================================================================

static volatile bool  pendingScoreUpdate = false;
static uint8_t        pendingScoreBuf[128];
static size_t         pendingScoreLen   = 0;

void onScoreWrite(const uint8_t* data, size_t len) {
    if (len > sizeof(pendingScoreBuf)) {
        len = sizeof(pendingScoreBuf);
    }
    memcpy(pendingScoreBuf, data, len);
    pendingScoreLen   = len;
    pendingScoreUpdate = true;
}

// ===========================================================================
// setup()
// ===========================================================================

void setup() {
    Serial.begin(115200);
    delay(200);                     // give Serial time to settle
    Serial.println();
    Serial.println("=== RallyTap-01 ===");

    // 1. Initialise display (enters BOOT state)
    displayManager.begin();
    displayManager.setState(DisplayManager::State::BOOT);
    displayManager.setScore(scoreManager);

    // 2. Initialise button handler
    buttonHandler.begin(18, 19);    // GPIO18 = A, GPIO19 = B

    // 3. Initialise BLE handler & register score-write callback
    bleHandler.begin();
    bleHandler.setScoreWriteCallback(onScoreWrite);

    Serial.println("[main] RallyTap-01 ready");
}

// ===========================================================================
// loop()
// ===========================================================================

void loop() {
    static bool wasConnected = false;

    // ---------------------------------------------------------------
    // 1. Poll buttons
    // ---------------------------------------------------------------
    PressResult press = buttonHandler.poll();

    if (press == PressResult::A) {
        Serial.println("[main] Button A pressed");
        bleHandler.notifyButtonPress(0x01);         // 0x01 = Player A
        displayManager.setState(DisplayManager::State::PRESSING);
    } else if (press == PressResult::B) {
        Serial.println("[main] Button B pressed");
        bleHandler.notifyButtonPress(0x02);         // 0x02 = Player B
        displayManager.setState(DisplayManager::State::PRESSING);
    }

    // ---------------------------------------------------------------
    // 2. BLE connection state tracking
    // ---------------------------------------------------------------
    bool connected = bleHandler.isConnected();

    if (connected && !wasConnected) {
        // Just connected
        Serial.println("[main] BLE connected");
        displayManager.setState(DisplayManager::State::CONNECTED);
    } else if (!connected && wasConnected) {
        // Just disconnected
        Serial.println("[main] BLE disconnected — entering reconnect");
        displayManager.setState(DisplayManager::State::RECONNECTING);
        // Advertising is already restarted by BLEHandler::onDisconnect,
        // but an explicit call ensures it runs immediately:
        bleHandler.startAdvertising();
    }
    wasConnected = connected;

    // ---------------------------------------------------------------
    // 3. Process pending score writes (from BLE score_display)
    // ---------------------------------------------------------------
    if (pendingScoreUpdate) {
        pendingScoreUpdate = false;                 // consume the flag

        String json(reinterpret_cast<char*>(pendingScoreBuf),
                    pendingScoreLen);

        Serial.print("[main] Score write: ");
        Serial.println(json);

        if (scoreManager.fromJSON(json)) {
            // Valid JSON — update display
            displayManager.setScore(scoreManager);

            if (scoreManager.getStatus() == "error") {
                displayManager.setState(DisplayManager::State::ERROR);
            } else {
                displayManager.setState(DisplayManager::State::CONFIRMING);
            }
        }
        // Invalid JSON: silently ignored (per R3 spec)
    }

    // ---------------------------------------------------------------
    // 4. Tick the display state machine (timed transitions)
    // ---------------------------------------------------------------
    displayManager.tick();

    // Small yield for watchdog / BLE background tasks
    delay(5);
}
