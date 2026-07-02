#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <BLE2902.h>

// BLE GATT service UUID
#define BLE_SERVICE_UUID        "f000a001-0451-4000-b000-000000000000"
#define BLE_CHAR_NAME           "f000a002-0451-4000-b000-000000000000"
#define BLE_CHAR_BUTTON_PRESS   "f000a003-0451-4000-b000-000000000000"
#define BLE_CHAR_SCORE_DISPLAY  "f000a004-0451-4000-b000-000000000000"

class BLEHandler {
public:
    BLEHandler();

    /// Initialise BLE device, create GATT server & characteristics, start advertising.
    void begin();

    /// Send a button-press notification to the connected client.
    /// @param player  0x01 = Player A, 0x02 = Player B
    void notifyButtonPress(uint8_t player);

    /// Register a callback invoked when the remote client writes to score_display.
    /// The callback receives the raw bytes and length of the written value.
    /// Called from the BLE FreeRTOS task — keep it fast and non-blocking.
    void setScoreWriteCallback(void (*callback)(const uint8_t* data, size_t len));

    /// Returns true when a BLE client is connected.
    bool isConnected() const;

    /// Restart advertising (e.g. after disconnect).
    void startAdvertising();

private:
    BLEServer*          _server;
    BLECharacteristic*  _buttonPressChar;
    BLECharacteristic*  _scoreDisplayChar;
    BLEAdvertising*     _advertising;
    bool                _connected;

    void (*_scoreWriteCallback)(const uint8_t*, size_t);

    // Static instance pointer for C-style callbacks
    static BLEHandler* _instance;

    // --- Nested callback classes ---

    class ServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* server) override;
        void onDisconnect(BLEServer* server) override;
    };

    class ScoreWriteCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic* characteristic) override;
    };
};

#endif // BLE_HANDLER_H
