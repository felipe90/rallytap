/**
 * RallyTap Phase 2 — Firmware Orchestrator (WiFi-Direct)
 *
 * ESP32-WROOM-32 / Arduino Framework
 *
 * The tap is a dumb WS client: devId + buttons + OLED (FW). The hub owns
 * binding, score authority, and the E18 priority rule. The phone-bridge
 * path is retired (REM-2).
 *
 * Data flow (one loop iteration):
 *   1. Drive WiFiHandler (AP connect/reconnect + WS client + register)
 *   2. Dispatch downlinks (bind/unbound/match/score) to ScoreManager/DisplayManager
 *   3. Poll buttons — enqueue {devId,button} presses when a match is active
 *   4. Flush the press FIFO over the WS (live + on reconnect, AC2)
 *   5. Tick the display state machine (auto-advances + idle → SLEEP)
 */

#include <Arduino.h>
#include <esp_system.h>

#include "ScoreManager.h"
#include "ButtonHandler.h"
#include "WiFiHandler.h"
#include "DisplayManager.h"

// ===========================================================================
// Installation configuration (hardcoded at install — SoftAP provisioning is
// Phase 2.1). Multi-profile: the tap rotates APs until one connects. The
// deploy profile joins the hub's existing open AP (`RallyOS`, wpa=0) instead
// of a dedicated tap SSID (AC-4 relaxed by user decision); the dev profile
// is the lab/local AP used for hardware-in-the-loop on the dev machine.
// ===========================================================================

static const TapNetworkProfile NETWORK_PROFILES[] = {
    // Deploy — hub's open AP (setup-orangepi-ap.sh: AP_SSID=RallyOS, wpa=0)
    { "RallyOS", "", "192.168.4.1", 3001 },
    // Dev / HIL — local AP on the dev machine's LAN
    { "TIMELINE-56", "Eraso1648", "192.168.20.57", 3001 },
};

static const char* FW_VERSION  = "2.0.0";           // reported in register

// ===========================================================================
// Global instances
// ===========================================================================

ScoreManager    scoreManager;
ButtonHandler   buttonHandler;
WiFiHandler     wifiHandler;
DisplayManager  displayManager;

static bool matchActive = false;   // true while the bound court has an active match

// ===========================================================================
// Call-sign (BND-5: 4 chars from the MAC; the full MAC is never shown)
// ===========================================================================

static String callSign() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char cs[5];
    snprintf(cs, sizeof(cs), "%02X%02X", mac[4], mac[5]);
    return String(cs);
}

// ===========================================================================
// Downlink dispatch (hub -> tap)
//
// Runs from loop() via the WiFiHandler callback. Any incoming frame wakes the
// display (E13). The tap renders exactly what it receives — NO swap logic
// (CONF-3).
// ===========================================================================

void onDownlink(const String& json) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json)) {
        return;
    }

    const char* type = doc["type"] | "";

    if (strcmp(type, "rallytap.bind") == 0) {
        // PROTO-1 — register reply carrying the live match state.
        matchActive = !doc["match"].isNull();
        scoreManager.fromJSON(json);
        displayManager.setScore(scoreManager);
        displayManager.setMesaId(doc["mesaId"] | "");
        displayManager.setState(DisplayManager::State::CONNECTED);
    } else if (strcmp(type, "rallytap.unbound") == 0) {
        // BND-5 — pairing affordance with the 4-char call-sign.
        matchActive = false;
        displayManager.setCallSign(callSign());
        displayManager.setState(DisplayManager::State::UNBOUND);
    } else if (strcmp(type, "rallytap.match") == 0) {
        // MATCH-1 — lifecycle push (start / end / state change).
        matchActive = !doc["match"].isNull();
        scoreManager.fromJSON(json);
        displayManager.setScore(scoreManager);
        if (matchActive) {
            displayManager.setState(DisplayManager::State::CONNECTED);
        } else if (strcmp(doc["score"]["status"] | "FINISHED", "FINISHED") == 0) {
            // MATCH-2 — the tap follows the match engine, not club flow state.
            displayManager.setState(DisplayManager::State::FINISHED);
        } else {
            displayManager.setState(DisplayManager::State::CONNECTED);
        }
    } else if (strcmp(type, "rallytap.score") == 0) {
        // MATCH-1 — live score push.
        scoreManager.fromJSON(json);
        displayManager.setScore(scoreManager);
        if (matchActive) {
            displayManager.setState(DisplayManager::State::CONNECTED);
        }
    }
    // Unknown frames are ignored.
}

// ===========================================================================
// setup()
// ===========================================================================

void setup() {
    Serial.begin(115200);
    delay(200);                     // give Serial time to settle
    Serial.println();
    Serial.println("=== RallyTap-01 (WiFi-Direct) ===");

    // 1. Initialise display (enters BOOT state; auto-advances to CONNECTING)
    displayManager.begin();
    displayManager.setState(DisplayManager::State::BOOT);
    displayManager.setScore(scoreManager);

    // 2. Initialise button handler + tap identity
    buttonHandler.begin(18, 19);    // GPIO18 = A, GPIO19 = B
    String cs    = callSign();
    String devId = String("dev-tap-") + cs;
    buttonHandler.setDevId(devId);
    displayManager.setCallSign(cs);

    // 3. Initialise WiFiHandler (STA + WS client + register on connect).
    //    Rotates through NETWORK_PROFILES until one AP links (FW-1 multi-AP).
    wifiHandler.begin(NETWORK_PROFILES,
                      sizeof(NETWORK_PROFILES) / sizeof(NETWORK_PROFILES[0]),
                      devId, cs, FW_VERSION);
    wifiHandler.setDownlinkCallback(onDownlink);

    Serial.print("[main] RallyTap-01 ready — devId ");
    Serial.println(devId);
}

// ===========================================================================
// loop()
// ===========================================================================

void loop() {
    static bool wasWsConnected = false;
    static bool wasFatal       = false;

    // ---------------------------------------------------------------
    // 1. WiFi / WS lifecycle (connect, backoff, heartbeat, re-register)
    // ---------------------------------------------------------------
    wifiHandler.loop();

    // ---------------------------------------------------------------
    // 2. Fatal wrong-AP — freeze CONNECTING on "Wrong AP" (FW-1/E6)
    // ---------------------------------------------------------------
    if (wifiHandler.isFatal()) {
        if (!wasFatal) {
            displayManager.setWrongAp(true);
            displayManager.setState(DisplayManager::State::CONNECTING);
            wasFatal = true;
        }
    } else {
        bool wsConnected = wifiHandler.isConnected();
        if (!wsConnected && wasWsConnected) {
            Serial.println("[main] WS dropped — entering reconnect");
            displayManager.setState(DisplayManager::State::RECONNECTING);
        }
        wasWsConnected = wsConnected;
    }

    // ---------------------------------------------------------------
    // 3. Buttons — wake, and enqueue {devId,button} only for an active match
    // ---------------------------------------------------------------
    PressResult press = buttonHandler.poll();
    if (press == PressResult::A || press == PressResult::B) {
        if (matchActive) {
            buttonHandler.enqueue(press);
            displayManager.setState(DisplayManager::State::PRESSING);
        } else {
            // E15 — press during SLEEP / no active match: wake, no-op.
            displayManager.wake();
            displayManager.setState(DisplayManager::State::CONNECTED);
        }
    }

    // ---------------------------------------------------------------
    // 4. Flush buffered presses over the WS (live + on reconnect, AC2)
    // ---------------------------------------------------------------
    if (wifiHandler.isConnected()) {
        String evt;
        while (buttonHandler.dequeue(evt)) {
            wifiHandler.send(evt);
        }
    }

    // ---------------------------------------------------------------
    // 5. Tick the display state machine (auto-advances + idle -> SLEEP)
    // ---------------------------------------------------------------
    displayManager.tick();

    // Small yield for WiFi/WS background tasks
    delay(5);
}
