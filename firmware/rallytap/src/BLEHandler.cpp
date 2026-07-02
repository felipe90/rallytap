#include "BLEHandler.h"

// Static instance
BLEHandler* BLEHandler::_instance = nullptr;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

BLEHandler::BLEHandler()
    : _server(nullptr)
    , _buttonPressChar(nullptr)
    , _scoreDisplayChar(nullptr)
    , _advertising(nullptr)
    , _connected(false)
    , _scoreWriteCallback(nullptr)
{
    _instance = this;
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void BLEHandler::begin() {
    // 1. Init BLE device
    BLEDevice::init("RallyTap-01");

    // 2. Create server
    _server = BLEDevice::createServer();
    _server->setCallbacks(new ServerCallbacks());

    // 3. Create the GATT service
    BLEService* service = _server->createService(BLE_SERVICE_UUID);

    // 4. Characteristic: device name (READ)
    BLECharacteristic* nameChar = service->createCharacteristic(
        BLE_CHAR_NAME,
        BLECharacteristic::PROPERTY_READ
    );
    nameChar->setValue("RallyTap-01");

    // 5. Characteristic: button press (NOTIFY) — requires CCCD descriptor
    _buttonPressChar = service->createCharacteristic(
        BLE_CHAR_BUTTON_PRESS,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _buttonPressChar->addDescriptor(new BLE2902());

    // 6. Characteristic: score display (WRITE)
    _scoreDisplayChar = service->createCharacteristic(
        BLE_CHAR_SCORE_DISPLAY,
        BLECharacteristic::PROPERTY_WRITE
    );
    _scoreDisplayChar->setCallbacks(new ScoreWriteCallbacks());

    // 7. Start the service
    service->start();

    // 8. Configure advertising
    _advertising = BLEDevice::getAdvertising();
    _advertising->addServiceUUID(BLE_SERVICE_UUID);
    _advertising->setScanResponse(true);
    _advertising->setMinPreferred(0x06);   // helps iOS connections
    _advertising->setMaxPreferred(0x12);

    // 9. Start advertising
    BLEDevice::startAdvertising();
}

// ---------------------------------------------------------------------------
// Notify
// ---------------------------------------------------------------------------

void BLEHandler::notifyButtonPress(uint8_t player) {
    if (_connected && _buttonPressChar != nullptr) {
        _buttonPressChar->setValue(&player, 1);
        _buttonPressChar->notify();
    }
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void BLEHandler::setScoreWriteCallback(void (*callback)(const uint8_t*, size_t)) {
    _scoreWriteCallback = callback;
}

// ---------------------------------------------------------------------------
// Connection state
// ---------------------------------------------------------------------------

bool BLEHandler::isConnected() const {
    return _connected;
}

// ---------------------------------------------------------------------------
// Advertising
// ---------------------------------------------------------------------------

void BLEHandler::startAdvertising() {
    if (_advertising != nullptr) {
        _advertising->start();
    }
}

// ===========================================================================
// ServerCallbacks
// ===========================================================================

void BLEHandler::ServerCallbacks::onConnect(BLEServer* /*server*/) {
    _instance->_connected = true;
}

void BLEHandler::ServerCallbacks::onDisconnect(BLEServer* /*server*/) {
    _instance->_connected = false;

    // Restart advertising so a new client can find us
    _instance->startAdvertising();
}

// ===========================================================================
// ScoreWriteCallbacks
// ===========================================================================

void BLEHandler::ScoreWriteCallbacks::onWrite(BLECharacteristic* characteristic) {
    if (_instance == nullptr || _instance->_scoreWriteCallback == nullptr) {
        return;
    }

    std::string value = characteristic->getValue();
    if (value.length() > 0) {
        _instance->_scoreWriteCallback(
            reinterpret_cast<const uint8_t*>(value.data()),
            value.length()
        );
    }
}
