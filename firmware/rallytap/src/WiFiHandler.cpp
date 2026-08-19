#include "WiFiHandler.h"
#include <ArduinoJson.h>
#include "NetworkProfiles.h"

// Static instance
WiFiHandler* WiFiHandler::_instance = nullptr;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

WiFiHandler::WiFiHandler()
    : _profiles(nullptr)
    , _profileCount(0)
    , _profileIndex(0)
    , _devId("")
    , _label("")
    , _fw("")
    , _phase(Phase::INIT)
    , _attempt(0)
    , _phaseStartMs(0)
    , _beginMs(0)
    , _wsInitialized(false)
    , _wsConnected(false)
    , _downlinkCb(nullptr)
{
    _instance = this;
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void WiFiHandler::begin(const TapNetworkProfile* profiles, size_t profileCount,
                        const String& devId, const String& label,
                        const char* fw) {
    _profiles     = profiles;
    _profileCount = (profileCount > 0) ? profileCount : 1;
    _profileIndex = 0;
    _devId        = devId;
    _label        = label;
    _fw           = fw;

    _attempt = 0;
    _beginMs = millis();
    startConnectAttempt();
}

// ---------------------------------------------------------------------------
// Main loop driver
// ---------------------------------------------------------------------------

void WiFiHandler::loop() {
    if (_phase == Phase::FATAL) {
        return;                     // E6 — stop oscillating once fatal
    }

    unsigned long now = millis();

    switch (_phase) {
        case Phase::CONNECTING: {
            if (WiFi.status() == WL_CONNECTED) {
                _attempt = 0;
                if (!_wsInitialized) {
                    initWebSocket();
                }
                _phase        = Phase::CONNECTED;
                _phaseStartMs = now;
            } else {
                unsigned long backoff = wifiBackoffMs(_attempt);
                if (now - _phaseStartMs >= backoff) {
                    // Give the current profile a few backoff ticks, then
                    // rotate to the next configured AP (multi-network
                    // fallback: deploy AP -> dev AP). FATAL only after the
                    // global timeout has passed without ANY profile linking.
                    if (wifiShouldRotateProfile(_attempt, PROFILE_ATTEMPTS_PER_PROFILE)) {
                        advanceProfile();
                    } else {
                        _attempt++;
                    }
                    if (now - _beginMs >= FATAL_TIMEOUT_MS) {
                        enterFatal();
                        return;
                    }
                    startConnectAttempt();
                }
            }
            break;
        }

        case Phase::CONNECTED: {
            if (WiFi.status() != WL_CONNECTED) {
                // AP dropped under us — retry the AP immediately, backoff
                // restarts from attempt 0.
                _attempt      = 0;
                _phaseStartMs = now;
                startConnectAttempt();
                return;
            }
            // Drives the WS socket, ping/pong heartbeat, and reconnect with
            // re-register (PROTO-1) via onWsEvent.
            _ws.loop();
            break;
        }

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Uplink / downlink
// ---------------------------------------------------------------------------

void WiFiHandler::send(const String& json) {
    if (_wsConnected) {
        _ws.sendTXT(json.c_str());
    }
}

void WiFiHandler::setDownlinkCallback(void (*cb)(const String&)) {
    _downlinkCb = cb;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void WiFiHandler::startConnectAttempt() {
    _phase        = Phase::CONNECTING;
    _phaseStartMs = millis();

    const TapNetworkProfile& profile = _profiles[_profileIndex % _profileCount];

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(profile.ssid, profile.psk);
}

const TapNetworkProfile& WiFiHandler::currentProfile() const {
    return _profiles[_profileIndex % _profileCount];
}

void WiFiHandler::advanceProfile() {
    _profileIndex = wifiNextProfileIndex(_profileIndex, _profileCount);
    _attempt      = 0;
    _phaseStartMs = millis();
}

void WiFiHandler::initWebSocket() {
    const TapNetworkProfile& profile = currentProfile();
    _ws.begin(profile.hubHost, profile.hubPort, "/");
    _ws.onEvent(WiFiHandler::onWsEvent);
    _ws.enableHeartbeat(WS_HEARTBEAT_MS, WS_HEARTBEAT_TO_MS, 0);  // WS-3
    _ws.setReconnectInterval(1000);
    _wsInitialized = true;
}

void WiFiHandler::sendRegister() {
    StaticJsonDocument<256> doc;
    doc["type"]  = "rallytap.register";
    doc["devId"] = _devId;
    doc["label"] = _label;
    doc["fw"]    = _fw;

    String out;
    serializeJson(doc, out);
    _ws.sendTXT(out.c_str());
}

void WiFiHandler::enterFatal() {
    _phase       = Phase::FATAL;
    _wsConnected = false;
}

// ---------------------------------------------------------------------------
// WebSocket events
// ---------------------------------------------------------------------------

void WiFiHandler::onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance != nullptr) {
        _instance->handleWsEvent(type, payload, length);
    }
}

void WiFiHandler::handleWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            // Re-run register on every connect / reconnect (PROTO-1).
            _wsConnected = true;
            sendRegister();
            break;

        case WStype_DISCONNECTED:
        case WStype_ERROR:
            _wsConnected = false;
            break;

        case WStype_TEXT:
            if (_downlinkCb != nullptr && payload != nullptr) {
                _downlinkCb(String(reinterpret_cast<char*>(payload)));
            }
            break;

        default:
            break;
    }
}
