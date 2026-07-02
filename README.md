# RallyTap

ESP32-based physical score button for padel/tennis referees. Connects to the referee's phone via **BLE GATT**, and the phone bridges button presses to rallyOS-hub via Web Bluetooth + Socket.IO.

```
Button A ── GPIO18 ─┐
                    ├── ESP32 ── BLE GATT ──► Phone Browser (Web Bluetooth) ── WS ── Server
Button B ── GPIO19 ─┘
```

## Hardware

| Component | Spec |
|-----------|------|
| MCU | ESP32-WROOM-32 (ESP32 Dev Module) |
| Display | SSD1306 128×64 OLED, I2C (SDA=GPIO21, SCL=GPIO22, addr 0x3C) |
| Button A | GPIO18 (INPUT_PULLUP), 50ms debounce |
| Button B | GPIO19 (INPUT_PULLUP), 50ms debounce |
| Power | USB-C (5V via dev board) |

## BLE GATT Protocol

| Characteristic | UUID | Type | Description |
|---|---|---|---|
| Service | `f000a001-0451-4000-b000-000000000000` | — | RallyTap primary service |
| Device Name | `f000a002` | READ | ASCII, max 16B — e.g. "RallyTap-01" |
| Button Press | `f000a003` | NOTIFY | 1B: `0x01` = Player A, `0x02` = Player B |
| Score Display | `f000a004` | WRITE | UTF-8 JSON, max 128B |

### Score Display JSON Format

```json
{"a":3, "b":1, "set_a":1, "set_b":0, "status":"ok", "msg":""}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `a` | int | 0 | Player A current points |
| `b` | int | 0 | Player B current points |
| `set_a` | int | 0 | Player A sets won |
| `set_b` | int | 0 | Player B sets won |
| `status` | string | "ok" | "ok" or "error" |
| `msg` | string | "" | Error message |

## Architecture

Two independent domains bridged by BLE GATT:

```
┌─────────────────────┐       BLE GATT        ┌──────────────────────┐
│   ESP32 Firmware     │ ◄──────────────────► │  Web Bridge (Client) │
│                     │                       │                      │
│  ButtonHandler      │  button_press notify  │  BLEBridge.ts        │
│  BLEHandler         │ ◄──────────────────►  │  useRallyTapBridge   │
│  DisplayManager     │  score_display write  │  RallyTapConnectBtn  │
│  ScoreManager       │                       │                      │
└─────────────────────┘                       └──────────┬───────────┘
                                                          │ Socket.IO
                                                          ▼
                                                  ┌────────────────┐
                                                  │  rallyOS-hub    │
                                                  │  Server         │
                                                  └────────────────┘
```

### Firmware (C++, PlatformIO)

- **ButtonHandler** — GPIO poll with 50ms debounce + 300ms cooldown
- **BLEHandler** — GATT server, button notify + score write callback
- **DisplayManager** — 7-state OLED machine (Boot/Idle/Connected/Pressing/Confirming/Error/Reconnecting)
- **ScoreManager** — JSON parse/format for score display writes

### Web Bridge (TypeScript, React)

- **BLEBridge** — Pure BLE abstraction: scan, connect, GATT subscribe, writeScore, auto-reconnect
- **useRallyTapBridge** — React hook wiring BLE bridge ↔ Socket.IO events
- **RallyTapConnectButton** — UI button + status badge (6 states)

## Build & Flash

### Prerequisites

- [PlatformIO](https://platformio.org/) CLI
- ESP32 connected via USB

### Commands

```bash
# Build firmware
cd firmware/rallytap
pio run

# Flash to ESP32
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

### Unit Tests

```bash
pio test -e native
```

Tests run natively on the host machine (no ESP32 required). Currently covers **ScoreManager** JSON parsing and display formatting.

## Documentation

Full architecture and protocol details:

- [`docs/physical-score-buttons.md`](https://github.com/rallyOS/rallyOS-hub/blob/main/docs/physical-score-buttons.md) — Full PoC architecture and BLE bridge design
- `firmware/rallytap/src/` — Firmware source with inline comments
- `test/test_score_manager.cpp` — Unit test suite

## Status

Phase 1 — Physical Score Button: **Complete** ✅

- [x] ESP32 firmware: buttons, BLE GATT, OLED display
- [x] Web Bluetooth bridge (BLEBridge + React hooks + UI)
- [x] Score write-back from hub to OLED
- [x] Unit tests for ScoreManager (9 tests, all passing)

## License

Proprietary — rallyOS project.
