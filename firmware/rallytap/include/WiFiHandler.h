#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsClient.h>
#include "WifiBackoff.h"

/// WiFi STA + thin plain-WebSocket client (FW-1, PROTO-4, WS-3).
///
/// Retires the Phase-1 GATT bridge: connects to the dedicated tap AP,
/// reconnects with exponential backoff + jitter, and runs the rallytap WS
/// client with a ping/pong heartbeat. On every (re)connect it re-runs the
/// rallytap.register handshake (PROTO-1) so the hub re-binds the tap
/// mid-match. A wrong or unreachable AP terminates in the FATAL phase after
/// FATAL_TIMEOUT_MS so the tap stops oscillating (E6) — main.cpp renders
/// "Wrong AP" on the OLED.
class WiFiHandler {
public:
    WiFiHandler();

    /// Configure the AP + hub endpoint and start connecting.
    /// devId/label/fw are reported in the register handshake.
    void begin(const char* ssid, const char* psk,
               const char* hubHost, uint16_t hubPort,
               const String& devId, const String& label,
               const char* fw);

    /// Must be called every loop(); drives AP connect/reconnect + WS client.
    void loop();

    /// True while the WS socket is up and register has been sent (PROTO-1).
    bool isConnected() const { return _wsConnected; }

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

private:
    enum class Phase : uint8_t {
        INIT,
        CONNECTING,     ///< attempting / backing off an AP connect
        CONNECTED,      ///< AP up; WS (re)connecting with re-register
        FATAL           ///< wrong AP — exhausted the fatal timeout
    };

    const char*  _ssid;
    const char*  _psk;
    const char*  _hubHost;
    uint16_t     _hubPort;
    String       _devId;
    String       _label;
    String       _fw;

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
    void initWebSocket();
    void sendRegister();
    void enterFatal();
    void handleWsEvent(WStype_t type, uint8_t* payload, size_t length);
    static void onWsEvent(WStype_t type, uint8_t* payload, size_t length);
    static WiFiHandler* _instance;
};

#endif // WIFI_HANDLER_H
