# RallyTap Protocol — firmware mirror

> **Status**: MIRROR of the canonical contract — version 1.
> The hub repo owns the canonical `shared/rallytap-protocol.md` +
> `shared/rallyTap.ts`. This file mirrors the wire envelope shapes for the
> firmware build, and the `[env:native]` fixture test
> (`test/test_protocol_fixture.cpp`) parses the SAME fixture JSON as the hub's
> `shared/__tests__/rallytap-protocol.test.ts`. A protocol-version bump MUST
> land in BOTH repos in one change (M1) — the shared-fixture test fails
> otherwise.

## Versioning

- `PROTOCOL_VERSION = 1` (mirrored const in the fixture test).
- The hub is backwards-tolerant of older tap firmware (M3): `register.fw`
  reports the tap firmware (this build: `"2.0.0"`); unknown-major fw is
  accepted with a WARN and downlink features absent from older fw are gated.

## Transport (firmware side)

- Plain WebSocket (NOT socket.io — PROTO-4). The tap opens a raw WS to
  `ws://<hub-static-ip>:<TAP_WS_PORT>/` (`TAP_WS_PORT` default **3001**).
- Ping/pong keepalive ~15–30 s (WS-3); the hub terminates a socket that stops
  answering pong, the tap reconnects and re-runs `register` (PROTO-1).
- The tap reaches the hub via the static IP on the dedicated tap SSID (WS-2);
  the SSID is separate from the guest network (AC-4).

## Envelopes

### Uplink (tap → hub)

#### `rallytap.register`

Sent on every connect and every reconnect (no tap restart).

```json
{ "type": "rallytap.register", "devId": "dev-tap-AC12", "label": "7B3D", "fw": "2.0.0" }
```

| Field | Type | Notes |
|---|---|---|
| `type` | `"rallytap.register"` | literal |
| `devId` | string | stable tap identity (`dev-tap-<call-sign>`) |
| `label` | string | short operator label (4-char call-sign) |
| `fw` | string | tap firmware version |

#### `rallytap.score` (button press)

The ONLY tap→hub event. **No `mesaId`, no `ts`** (PROTO-2/A7) — the tap has no
clock; the hub resolves the court from the binding and stamps arrival time.
An uplink carrying `mesaId`/`ts` is malformed and MUST be ignored by the hub.

```json
{ "type": "rallytap.score", "devId": "dev-tap-AC12", "button": "A" }
```

| Field | Type | Notes |
|---|---|---|
| `type` | `"rallytap.score"` | literal |
| `devId` | string | stable tap identity |
| `button` | `"A" \| "B"` | sport-agnostic side (E14) |

### Downlink (hub → tap)

The tap renders exactly what it receives — NO side-swap logic (CONF-3). The
hub side-maps `leftName`/`rightName` already.

#### `rallytap.bind` — register reply for a bound tap

Carries the CURRENT state of the mesa's active match (real score, never 0–0 —
PROTO-1/PERS-2/E1).

```json
{
  "type": "rallytap.bind",
  "mesaId": "court-3",
  "paired": true,
  "match": { "matchId": "M-42", "status": "LIVE", "winner": null, "score_a": 3, "score_b": 2, "set_a": 2, "set_b": 1 },
  "leftName": "Pedro",
  "rightName": "Juan",
  "score": { "a": 3, "b": 2, "set_a": 2, "set_b": 1, "status": "LIVE", "msg": "" }
}
```

| Field | Type | Notes |
|---|---|---|
| `type` | `"rallytap.bind"` | literal |
| `mesaId` | string | downlink-only `courtId` (`court-N`) — OLED shows `Mesa N` |
| `paired` | `true` | literal |
| `match` | `object \| null` | summary of the active match, null when none |
| `leftName` / `rightName` | `string \| null` | side-mapped names (CONF-3) |
| `score` | object | live score (`a`/`b` current, `set_a`/`set_b` sets won, `status`, `msg`) |

#### `rallytap.unbound` — register reply for an unbound tap

```json
{ "type": "rallytap.unbound", "reason": "no-binding" }
```

The tap renders its pairing affordance with its 4-char call-sign
(`Sin emparajar — Pair me 7B3D`, BND-5).

#### `rallytap.conflict` — duplicate register

```json
{ "type": "rallytap.conflict" }
```

#### `rallytap.match` — lifecycle push

Pushed at match start (summary + names + 0–0), match end (`match: null`), and
state changes (MATCH-1).

```json
{ "type": "rallytap.match", "mesaId": "court-3", "match": null, "leftName": null, "rightName": null, "score": { "a": 0, "b": 0, "set_a": 0, "set_b": 0, "status": "FINISHED", "msg": "" } }
```

#### `rallytap.score` — live score push

Pushed on every score change (MATCH-1). Names already side-mapped (CONF-3).

```json
{ "type": "rallytap.score", "mesaId": "court-3", "score": { "a": 3, "b": 2, "set_a": 2, "set_b": 1, "status": "LIVE", "msg": "" }, "leftName": "Pedro", "rightName": "Juan" }
```

## Fixture gate

`test/test_protocol_fixture.cpp` pins `PROTOCOL_VERSION = 1` and parses the
exact bind/register/button/match/score fixture JSON that the hub's
`shared/__tests__/rallytap-protocol.test.ts` pins. Bump both mirrors together
(M1).
