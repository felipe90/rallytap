#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsClient.h>
#include "WifiBackoff.h"

/// One candidate WiFi AP profile. The handler rotates through the profile
/// list until one connects; each profile carries its own hub endpoint so a
/// tap can move between the deploy AP (RallyOS) and a dev/lab AP.
struct TapNetworkProfile {
    const char* ssid;
    const char* psk;        ///< empty string = open network
    const char* hubHost;
    uint16_t    hubPort;
};

/// WiFi STA + thin plain-WebSocket client (FW-1, PROTO-4, WS-3).
///
/// Retires the Phase-1 GATT bridge: connects to one of the configured AP
/// profiles (rotating on failure), reconnects with exponential backoff +
/// jitter, and runs the rallytap WS client with a ping/pong heartbeat. On
/// every (re)connect it re-runs the rallytap.register handshake (PROTO-1) so
/// the hub re-binds the tap mid-match. If no profile can reach its AP within
/// FATAL_TIMEOUT_MS the tap enters the FATAL phase and stops oscillating
/// (E6) — main.cpp renders "Wrong AP" on the OLED.
class WiFiHandler {
public:
    WiFiHandler();

    /// Configure the AP profiles + hub endpoint and start connecting.
    /// devId/label/fw are reported in the register handshake.
    void begin(const TapNetworkProfile* profiles, size_t profileCount,
               const String& devId, const String& label,
               const char* fw);

    /// Must be called every loop(); drives AP connect/reconnect + WS client.
    void loop();

    /// True while the WS socket is up and register has been sent (PROTO-1).
    bool isConnected() const { return _wsConnected; }

    /// True while the STA link to the AP is up — drives the CONNECTING phase
    /// line (REQ-FB-4). Same probe WiFiHandler.cpp uses for phase transitions.
    bool isApUp() const { return WiFi.status() == WL_CONNECTED; }

    /// True after the fatal backoff timeout on a wrong/unreachable AP (E6).
    bool isFatal() const { return _phase == Phase::FATAL; }

    /// Send a tap->hub JSON frame (button score uplink).
    void send(const String& json);

    /// Register a callback invoked with each hub->tap JSON frame
    /// (rallytap.bind / unbound / match / score downlinks).
    void setDownlinkCallback(void (*cb)(const String& json));

    /// Timing constants (exposed for verification).
    static constexpr unsigned long WS_HEARTBEAT_MS     = 20000;  // WS-3 ping
    static constexpr unsigned long WS_HEARTBEAT_TO_MS  = 3000;   // pong timeout
    static constexpr unsigned long FATAL_TIMEOUT_MS    = 120000; // wrong AP
    static constexpr unsigned int  PROFILE_ATTEMPTS_PER_PROFILE = 4; // backoff ticks per AP before rotating

private:
    enum class Phase : uint8_t {
        INIT,
        CONNECTING,     ///< attempting / backing off an AP connect
        CONNECTED,      ///< AP up; WS (re)connecting with re-register
        FATAL           ///< wrong AP — exhausted the fatal timeout
    };

    const TapNetworkProfile* _profiles;
    size_t          _profileCount;
    size_t          _profileIndex;
    String          _devId;
    String          _label;
    String          _fw;

    Phase           _phase;
    unsigned int    _attempt;
    unsigned long   _phaseStartMs;
    unsigned long   _beginMs;
    bool            _wsInitialized;
    bool            _wsConnected;

    WebSocketsClient _ws;

    void (*_downlinkCb)(const String&);

    // --- helpers ---
    void startConnectAttempt();
    void advanceProfile();
    const TapNetworkProfile& currentProfile() const;
    void initWebSocket();
    void sendRegister();
    void enterFatal();
    void handleWsEvent(WStype_t type, uint8_t* payload, size_t length);
    static void onWsEvent(WStype_t type, uint8_t* payload, size_t length);
    static WiFiHandler* _instance;
};

#endif // WIFI_HANDLER_H
