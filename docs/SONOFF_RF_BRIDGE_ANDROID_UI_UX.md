# SONOFF RF Bridge — Android UI/UX Integration Contract

This document is the app-facing contract for the `SONOFF_RF` RF bridge firmware
implemented in `IotCloud_sonoff_rf/src/rf_bridge.cpp`.

It describes the MQTT topics, JSON payloads, state transitions, validation rules,
error handling, and the UI behavior required to learn and manage ordinary
fixed-code RF sensors and remotes.

The bridge was verified in the field with a door sensor. The sensor was learned
with code `1906529`, timings around `sync=15114`, `low=488`, `high=1418`, and
registered successfully with one reading per activation. These values are an
example only; the app must always use the candidate values received from the
bridge.

## 1. Product behavior

The bridge is not a generic RF database controlled entirely by the app. It owns
the RF registry in its LittleFS storage and performs the following operations:

- Receives stock iTead/Sonoff RF UART frames.
- Learns a code from ordinary received RF traffic during a 60-second software
  capture session.
- Requires three matching readings before a candidate can be registered.
- Stores up to five registered RF devices/signals.
- Publishes structured events only for registered codes.
- Can transmit a standard stock-MCU RF code for diagnostics or supported
  fixed-code devices.

The bridge does not support rolling codes, authenticated RF senders, Portisch
raw/bucket modes, or an inferred open/closed state. A door sensor's “open” and
“closed” transmissions, if they are different codes, must be registered as two
separate records.

## 2. MQTT topic model

The app must build topics from the device metadata already used by IoTCloud. Do
not hard-code the location, device ID, or sensor ID.

```text
BASE       = v1/<locationId>/<deviceId>/
SENSOR_ID  = <deviceInternalId>_001_RF
RF_BASE    = BASE + SENSOR_ID + "/aux/rf/"
```

For example:

```text
v1/<locationId>/<deviceId>/<deviceInternalId>_001_RF/aux/rf/
```

The bridge model is published at the device level as:

```text
BASE + "model" = "SONOFF_RF"
```

The RF sensor suffix is `001_RF` and its exported sensor type is `rf_bridge`.
The app should identify the RF bridge from the device model and use the actual
sensor topic supplied by device discovery.

### Command topics

Commands are published to one of these topics:

```text
RF_BASE + "initialize"
RF_BASE + "learn"
RF_BASE + "register"
RF_BASE + "delete"
RF_BASE + "cancel"
RF_BASE + "list"
RF_BASE + "send"
```

All command messages must be JSON objects, must contain a unique
`request_id` (1–64 characters), and must be published with `retain=false`.
The backend gateway publishes commands with QoS 1. Retaining a command could
cause it to run again whenever the bridge reconnects.

The `request_id` is echoed by the bridge in the corresponding `result` message.
The `request_id` used for a successful `learn` command also becomes the
`learning_session_id` for its candidate, register, and cancel messages.

### Output topics

The app should subscribe to the complete RF subtree while the bridge screen is
open, and preferably maintain the device-level status subscription globally:

```text
RF_BASE + "#"
```

The bridge publishes:

```text
RF_BASE + "status"     // retained
RF_BASE + "result"     // non-retained
RF_BASE + "candidate"  // non-retained
RF_BASE + "devices"    // non-retained
RF_BASE + "event"      // non-retained
```

Every telemetry payload includes:

```json
{
  "protocol_version": 1,
  "boot_id": "per-boot-identifier"
}
```

`boot_id` changes after a bridge reboot. The backend uses it to reject stale
registry snapshots and events from an earlier boot. Registry messages also
include `registry_revision` and `snapshot_id`; events include a monotonic
`sequence` beginning at 1 for each boot.

## 3. Screen and state model

The app should model bridge state separately from the list of registered
records. A useful state model is:

```text
Unknown
  -> StorageUninitialized       status.storage == "unmounted"
  -> Ready                      status.storage == "ready"
  -> StorageError               status.storage == "invalid_registry"

Ready
  -> Learning                   result(operation=learn, result=learning)
  -> Ready                      result(operation=cancel, result=cancelled)
  -> Ready                      result(operation=learn, result=expired)
  -> Registering                candidate.ready == true + register command
  -> Ready                      result(operation=register, result=registered)
```

Recommended screen sections:

1. **Connection and storage status**
   - Online/offline from the normal device status topic.
   - RF storage state from `status.storage`.
   - Registered count, displayed as `devices / 5`.
   - A warning when storage is uninitialized or invalid.

2. **Registered RF devices**
   - ID, friendly message, code, repeats, and debounce.
   - Refresh using the `list` command because device records are not retained.
   - Actions: learn/replace, delete, and optional test/diagnostic actions.

3. **Learn new device**
   - A 60-second countdown.
   - Human instructions appropriate for the device type.
   - Candidate progress and technical signal details.
   - Registration form enabled only when a candidate is ready.

4. **Activity and diagnostics**
   - Latest structured event.
   - Optional counters from `status`.
   - Optional advanced controls for raw fixed-code send.

### Learning UX for a door sensor

The RF bridge does not require a physical button on the sensor. For a door
contact, the app should say something like:

> Close and open the door three times. Keep the bridge connected and perform
> the activations within the countdown.

Do not tell the user to press a button unless the selected device actually has
one. “Activation” is the correct general term.

The bridge publishes a candidate for each of the first three matching readings.
The UI should show progress such as `1/3`, `2/3`, and `3/3 ready`.

Once `ready=true` is received, the app should enable registration immediately.
The registration command must be sent while the learning session is still
active. A ready candidate is held in RAM only; it is not persisted until
registration succeeds.

If the session expires, the app must explain that the signal was detected but
not saved, then offer **Start again**. It must not imply that the device was
registered merely because a candidate was seen.

## 4. Command and response contract

All examples below assume `RF_BASE` has already been resolved.

### 4.1 Read current registry: `list`

Publish:

```json
{
  "request_id": "list-bridge-001"
}
```

to:

```text
RF_BASE + "list"
```

The bridge publishes one message per device, followed by a completion marker.

Device record:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "registry_revision": 4,
  "snapshot_id": "a1b2c3d4e5f60718-2",
  "index": 0,
  "total": 1,
  "device": {
    "id": "door_open",
    "message": "Door opened",
    "repeats": 1,
    "debounce_ms": 3000,
    "code": 1906529,
    "sync": 15114,
    "low": 488,
    "high": 1418
  }
}
```

Completion marker:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "registry_revision": 4,
  "snapshot_id": "a1b2c3d4e5f60718-2",
  "total": 1,
  "complete": true
}
```

For an empty registry, the bridge publishes only:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "registry_revision": 4,
  "snapshot_id": "a1b2c3d4e5f60718-2",
  "total": 0,
  "complete": true
}
```

The app should clear or replace its list when a new list operation starts, use
`index` and `total` to assemble records, and consider the list authoritative
only after `complete=true`. All fragments must have the same `boot_id`,
`registry_revision`, and `snapshot_id`. A snapshot from a different boot or
revision must not replace the current registry.

### 4.2 Start learning: `learn`

Publish:

```json
{
  "request_id": "learn-bridge-001"
}
```

The normal success sequence is:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "learn-bridge-001",
  "operation": "learn",
  "result": "learning"
}
```

followed by a retained status update with:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "registry_revision": 4,
  "storage": "ready",
  "devices": 0,
  "learning": true,
  "learning_session_id": "learn-bridge-001",
  "received": 0,
  "unknown": 0,
  "rejected": 0,
  "published": 0,
  "transmitted": 0,
  "suppressed": 0,
  "parser_errors": 0
}
```

During capture, candidates look like:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "learning_session_id": "learn-bridge-001",
  "code": 1906529,
  "sync": 15114,
  "low": 488,
  "high": 1418,
  "readings": 1,
  "ready": false
}
```

The third matching reading becomes:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "learning_session_id": "learn-bridge-001",
  "code": 1906529,
  "sync": 15114,
  "low": 488,
  "high": 1418,
  "readings": 3,
  "ready": true
}
```

Important behavior:

- The capture window lasts 60 seconds.
- Up to eight distinct candidate codes can be observed in one window.
- Candidates must have the same code and timings within a 25% tolerance.
- Only ordinary received `A4` frames are learned.
- Candidate messages are non-retained. If the app misses them, restart
  learning rather than assuming a candidate exists.
- Starting learning again resets the candidate set.

Possible failures include:

```json
{"operation":"learn","result":"storage_error"}
{"operation":"learn","result":"expired"}
```

`expired` means the capture ended without a successful registration. It can
also occur when the MQTT connection is lost during learning.

### 4.3 Register a candidate: `register`

The app must send the `code` from a ready candidate. The bridge looks up the
candidate's timings internally; the app does not need to send `sync`, `low`, or
`high` in the registration payload.

Example:

```json
{
  "request_id": "register-bridge-001",
  "learning_session_id": "learn-bridge-001",
  "id": "door_open",
  "code": 1906529,
  "message": "Door opened",
  "repeats": 1,
  "debounce_ms": 3000
}
```

Fields:

| Field | Required | Rules |
|---|---:|---|
| `request_id` | Yes | Unique command ID, 1–64 characters; echoed by `result` |
| `learning_session_id` | Yes | The `request_id` from the active successful `learn` command |
| `id` | Yes | 1–32 ASCII letters, digits, `_`, or `-` |
| `code` | Yes | Decimal integer from 0 through 16,777,215 |
| `message` | No | Up to 96 UTF-8 bytes; omitted defaults to `id`; optional display metadata |
| `repeats` | No | Integer 1–5; default 2 |
| `debounce_ms` | No | Integer 100–60,000; default 3,000 |

On success:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "register-bridge-001",
  "operation": "register",
  "result": "registered"
}
```

The bridge then publishes the updated device list.

Registration with an existing `id` replaces that record, but it still requires
a fresh ready candidate. A code cannot be assigned to two different IDs.

Suggested defaults:

- Use `repeats=1` for a sensor that emits one RF frame per activation, such as
  the door sensor observed during integration.
- Use `repeats=2` for a transmitter known to repeat each activation, or leave
  the field omitted to use the firmware default.
- Use `debounce_ms=3000` unless the product needs a different event cadence.

The app must not declare success from the local form submission. Wait for the
`registered` result and then refresh/assemble the `devices` stream.

### 4.4 Cancel learning: `cancel`

Publish:

```json
{
  "request_id": "cancel-bridge-001",
  "learning_session_id": "learn-bridge-001"
}
```

Success:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "cancel-bridge-001",
  "operation": "cancel",
  "result": "cancelled"
}
```

Cancel should be available from the learning screen. It should also be sent
when the user explicitly leaves the flow, unless the app already received a
registration result.

### 4.5 Delete a registered device: `delete`

Publish:

```json
{
  "request_id": "delete-bridge-001",
  "id": "door_open"
}
```

Success:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "delete-bridge-001",
  "operation": "delete",
  "result": "deleted"
}
```

The bridge follows success with a fresh device list. The app should show a
confirmation dialog because deleting removes the persistent registration.

Possible result:

```json
{"protocol_version":1,"boot_id":"a1b2c3d4e5f60718","request_id":"delete-bridge-001","operation":"delete","result":"not_found"}
```

### 4.6 Initialize RF storage: `initialize`

This is a setup/recovery operation, not a normal screen action. It formats the
reserved LittleFS partition and erases every RF registration. It does not erase
the IoTCloud device configuration.

Only expose it when status reports `storage="unmounted"`, or behind an
advanced recovery action after clearly explaining the data loss.

Publish exactly:

```json
{
  "request_id": "initialize-bridge-001",
  "confirm": "ERASE_RF_REGISTRY"
}
```

Success:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "initialize-bridge-001",
  "operation": "initialize",
  "result": "initialized"
}
```

Other results:

```json
{"protocol_version":1,"boot_id":"a1b2c3d4e5f60718","request_id":"initialize-bridge-001","operation":"initialize","result":"confirmation_required"}
{"protocol_version":1,"boot_id":"a1b2c3d4e5f60718","request_id":"initialize-bridge-001","operation":"initialize","result":"storage_error"}
```

After initialization, request `list` and expect an empty completion marker.

### 4.7 RF transmit test: `send`

This is an advanced diagnostic for a code whose timings are already known:

```json
{
  "request_id": "send-bridge-001",
  "sync": 15114,
  "low": 488,
  "high": 1418,
  "code": 1906529
}
```

The success result is:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "send-bridge-001",
  "operation": "send",
  "result": "sent",
  "delivery": "rf_mcu_uart",
  "receiver_confirmed": false
}
```

`receiver_confirmed=false` is intentional. It only means the bridge wrote the
transmit bytes to its RF MCU; it cannot confirm that the remote receiver acted.
The app must never present this as a confirmed device action.

Transmission is rejected while learning:

```json
{"protocol_version":1,"boot_id":"a1b2c3d4e5f60718","request_id":"send-bridge-001","operation":"send","result":"learning_active"}
```

Other invalid payloads return `invalid_signal`.

## 5. Status schema

`status` is retained and is published on state change by the bridge. The fields
are diagnostic counters and reset on reboot.

| Field | Meaning |
|---|---|
| `protocol_version` | RF MQTT contract version; currently `1` |
| `boot_id` | Identifier generated for the current bridge boot |
| `registry_revision` | Monotonic registry revision for this boot |
| `storage` | `unmounted`, `ready`, or `invalid_registry` |
| `devices` | Number of registered records, maximum 5 |
| `learning` | Whether the 60-second capture session is active |
| `learning_session_id` | Present while learning; identifies the active learn request |
| `received` | RF frames received by the bridge |
| `unknown` | Valid frames whose code did not match a registered record |
| `rejected` | Invalid or timing-mismatched frames |
| `published` | Structured registered events published |
| `transmitted` | RF transmit commands written to the RF MCU |
| `suppressed` | Matching echo frames suppressed after a transmit |
| `parser_errors` | UART parser/resynchronization errors |

The UI should display `storage`, `devices`, and `learning` prominently. Counters
can be placed under an advanced diagnostics section.

Treat `invalid_registry` as a serious error. Event matching is disabled and
writes are refused until the registry is recovered or explicitly initialized.

## 6. Result schema and error handling

Every command response on `RF_BASE/result` has at least:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "request_id": "request-identifier",
  "operation": "<command>",
  "result": "<state>"
}
```

`send` additionally includes `delivery` and `receiver_confirmed`.

The Android client should match results by `request_id`, while still serializing
mutating operations per bridge. Do not allow overlapping `learn`, `register`,
`delete`, or `initialize` operations from multiple screens.

Recognized result values and recommended UI behavior:

| Result | Meaning | UI action |
|---|---|---|
| `learning` | Capture started | Show countdown and activation instructions |
| `listed` | Registry request accepted | Assemble the following `devices` snapshot |
| `cancelled` | Capture stopped by user | Return to ready state |
| `expired` | Capture timed out or MQTT disconnected | Show “detected but not saved”; offer restart |
| `registered` | Record persisted | Show success, refresh list, offer test |
| `deleted` | Record removed | Remove after list refresh |
| `not_found` | Requested ID does not exist | Refresh list and show a nonfatal error |
| `candidate_needs_three_readings` | Code was not ready | Continue capture or restart |
| `no_learning_session` | Registration arrived too late | Restart capture; do not retry register with stale candidate |
| `code_already_registered` | Code belongs to another ID | Show existing conflict and ask user to choose another record |
| `registry_full` | Five records already exist | Require deletion before adding another |
| `invalid_id` | ID violates naming rules | Highlight the ID field |
| `invalid_code` | Code missing or out of range | Treat as an app bug or stale candidate |
| `invalid_registration` | Field validation failed | Highlight invalid registration data |
| `storage_error` | Registry unavailable or write failed | Show persistent hardware/storage error |
| `invalid_json` | Malformed MQTT payload | Treat as client integration error |
| `invalid_request_id` | Missing or oversized command ID | Treat as a client integration error |
| `invalid_learning_session` | Registration/cancel refers to another learn session | Discard the stale flow and restart learning |
| `confirmation_required` | Initialize confirmation was wrong/missing | Keep storage recovery blocked |
| `initialized` | Registry formatted | Refresh status and list |
| `learning_active` | Transmit blocked during capture | Wait/cancel learning first |
| `invalid_signal` | Send signal is invalid | Highlight signal fields |
| `sent` | Bytes written to RF MCU | Explain that receiver action is unconfirmed |

If no result arrives, distinguish MQTT connection loss from a firmware error.
Reconnect, refresh `status`, and request `list`; do not blindly repeat
`initialize`.

## 7. Event schema and event UX

For a registered matching RF activation, the bridge publishes:

```json
{
  "protocol_version": 1,
  "boot_id": "a1b2c3d4e5f60718",
  "sequence": 1,
  "id": "door_open",
  "message": "Door opened",
  "uptime_ms": 596900,
  "occurred_at": "2026-09-10T19:42:03Z",
  "code": 1906529,
  "sync": 15146,
  "low": 488,
  "high": 1422
}
```

The app should:

- Route the event to the registered record by `id`.
- Reject events with an unsupported `protocol_version`.
- Reject events whose `boot_id` does not match the current registry/status.
- Dispatch each `(boot_id, sequence)` at most once and reject sequences that are
  not greater than the last accepted sequence for that boot.
- Update activity/history and any user-facing automation state.
- Use `message` as the human-readable label when non-empty.
- Use `occurred_at` (ISO 8601 UTC timestamp) as the exact wall-clock time the event was detected. If NTP was not yet synchronized at the moment of detection, `occurred_at` will be `null`.
- Treat `uptime_ms` as bridge uptime in milliseconds since boot.
- Avoid deriving a door state from the event unless the registered record has
  an explicit semantic such as “open” or “closed”.

Matching rules implemented by the bridge:

- Code must match exactly.
- `sync`, `low`, and `high` must be within 25% of the registered timings.
- The configured number of readings must arrive within one second.
- No gap between readings may exceed 500 ms.
- The per-record debounce interval prevents repeated events.
- A held transmitter can produce at most one event per debounce period.
- A transmit echo within one second is suppressed.

## 8. App-side validation and data types

The app should validate before publishing, while still handling firmware
validation errors defensively.

Recommended types:

- `code`: unsigned 32-bit integer in the transport model, constrained to
  `0..0xFFFFFF`.
- `sync`, `low`, `high`: unsigned 16-bit integers.
- `repeats`: integer `1..5`.
- `debounceMs`: integer `100..60000`.
- `durationMs`: integer `20..2000`.
- `id`: ASCII string, not a localized display name.
- `message`: UTF-8 string whose encoded byte length is at most 96.

Keep the stable machine ID separate from the editable display label. A good
pattern is:

```text
id       = door_open
message  = Front door opened
```

The ID is used for later delete/replace operations. It should not be silently
regenerated when the user edits the display label.

## 9. Connectivity and synchronization rules

The MQTT behavior has several consequences for Android:

- Commands are non-retained. Subscribe before publishing commands whenever a
  response is required.
- `status` is retained, but counters may reflect an earlier boot. Use it for
  current storage/learning hints, not as an event history.
- `candidate`, `result`, `devices`, and `event` are non-retained. They can be
  missed while the app is disconnected.
- Request `list` when opening the RF screen and after reconnecting.
- Request `status` only if the app's normal MQTT/device layer supports a
  status refresh; otherwise use the retained status and refresh the screen on
  reconnect.
- If `learning=true` is not seen or later changes to false before registration,
  show that the session ended and restart learning.
- Never restore a pending command by republishing a retained command.
- Only one registration flow should be active per bridge.

The app should use a small per-bridge command coordinator:

```text
generate request_id and publish command
  -> wait for result where result.request_id == request_id
  -> apply result
  -> for register/delete/initialize, request or await list/status refresh
```

Use a reasonable response timeout and show a retry action. For `learn`, keep a
local 60-second deadline, but treat the bridge's `expired` result as
authoritative if it arrives first.

## 10. Recommended end-to-end flows

### First-time bridge setup

1. Discover a device with model `SONOFF_RF`, sensor type `rf_bridge`, and sensor suffix `_001_RF`.
2. Subscribe to `RF_BASE/#`.
3. Read retained `status`.
4. If storage is `unmounted`, show a destructive confirmation explaining that
   RF registrations will be erased.
5. Publish `initialize` with the exact confirmation string.
6. Wait for `initialized` and `storage=ready`.
7. Request `list` and show an empty registry.

### Learn a door sensor

1. Confirm the bridge is online and storage is `ready`.
2. Confirm the registry is not full.
3. Publish `learn` with a unique `request_id`.
4. Start the 60-second UI countdown only after receiving
   `result=learning`.
5. Tell the user to close/open the door three times.
6. Display candidate progress from `readings`.
7. When `ready=true`, immediately show the registration form and publish
   `register` with the learn command's `learning_session_id`.
8. Use `repeats=1` when the sensor produces one frame per activation; otherwise
   use the user's selected repeat setting.
9. Wait for `result=registered`.
10. Assemble the following `devices` messages through `complete=true`.
11. Ask the user to perform one more activation and confirm a matching `event`.

### Replace an existing record

1. Start a new `learn` session.
2. Capture three readings from the replacement transmitter.
3. Register using the existing `id`.
4. Treat `registered` as an atomic replacement; do not delete first.

### Delete a record

1. Ask for confirmation.
2. Publish `delete` with the stable ID.
3. Wait for `deleted`.
4. Refresh/assemble the device list.

## 11. Test checklist for the Android implementation

### Discovery and setup

- [ ] `SONOFF_RF` is recognized as an RF bridge.
- [ ] The app builds `RF_BASE` from metadata rather than a hard-coded topic.
- [ ] Retained `storage=unmounted` shows the setup/recovery UI.
- [ ] Initialize requires the exact confirmation and visibly warns about erasure.
- [ ] A ready empty registry is displayed after initialization.

### Learning

- [ ] Learn starts only when storage is ready.
- [ ] The screen supports buttonless devices such as door sensors.
- [ ] Countdown starts after `result=learning`.
- [ ] Candidate progress shows 1/3, 2/3, and 3/3.
- [ ] The register action is disabled until `ready=true`.
- [ ] Registration is sent immediately after readiness.
- [ ] Expiry explains “detected but not saved” and offers restart.
- [ ] Cancel works and returns to the ready state.

### Registry management

- [ ] List assembly waits for `complete=true`.
- [ ] Empty lists are supported.
- [ ] IDs and message lengths are validated locally.
- [ ] Existing IDs can be replaced only from a fresh candidate.
- [ ] Duplicate code and full-registry errors are actionable.
- [ ] Delete requires confirmation and refreshes the list.

### Events and diagnostics

- [ ] Registered events update the correct record.
- [ ] Structured RF events are consumed from `.../aux/rf/event`.
- [ ] Debounce behavior is represented correctly in the UI.
- [ ] Send clearly says receiver action is not confirmed.
- [ ] MQTT reconnect triggers status/list resynchronization.
- [ ] No MQTT command is published with `retain=true`.

## 12. Firmware source of truth

When this document and the app implementation disagree with firmware behavior,
use these files as the source of truth:

- `IotCloud_sonoff_rf/src/rf_bridge.cpp`
- `IotCloud_sonoff_rf/src/rf_bridge.h`
- `IotCloud_sonoff_rf/src/rf_protocol.h`
- `IotCloud_sonoff_rf/README`

The bridge's MQTT contract is intentionally small and JSON-based. Keep the
Android implementation tolerant of unknown JSON fields so the firmware can add
diagnostic fields without breaking the app.
