# Sonoff Dual R3 Lite — BLE presence scanner

ESP32 firmware for a Sonoff Dual R3 Lite installed at the house entry. It
listens for the BLE presence beacons that household phones advertise and
forwards what it hears over MQTT. It holds no secrets and makes no presence
decisions: the API resolves each token to a user and decides enter/leave.

The two relays still work as normal IotCloud switches.

```text
phone --BLE adv--> this device --MQTT aux/ble/sighting--> automation_runner
  --POST /ble-sightings--> API (resolve token, hysteresis, enter/leave events)
```

## Sensors

| Sensor ID suffix | Type | Purpose |
| --- | --- | --- |
| `Switch1` | `switch` | RELAY_1 (GPIO27), toggled by SWITCH_1 (GPIO32) |
| `Switch2` | `switch` | RELAY_2 (GPIO14), toggled by SWITCH_2 (GPIO33) |
| `001_BLE` | `ble_scanner` | BLE presence scanner |

Button GPIO0 is the IotCloud reset/provisioning button; GPIO13 is the status
LED. Pins come from the `sonoff_dualr3` board variant.

## Before provisioning

Update **every household phone** to an app build that knows the
`ble_scanner` sensor type. Older builds can't parse it.

## Flashing

The firmware uses the `min_spiffs.csv` partition table (two 1.9 MB app slots)
because NimBLE doesn't fit the default 1.25 MB slots. A partition table can't
change over OTA, so flash over serial the first time; later updates can use OTA.

```sh
pio run -e esp32dev -t upload
```

Then provision with the usual IotCloud setup flow.

## What the scanner does

- Continuous passive scan, duplicate filtering off, no results stored
  (phones randomize their MAC address).
- Keeps only manufacturer data that is exactly 13 bytes and starts with
  `FF FF 49 43 01` (company `0xFFFF`, `"IC"`, protocol version 1). The last
  8 bytes are the rolling token.
- Per token (lowercase hex): averages RSSI and publishes the mean at most once
  every 2 s while heard. After 20 s of silence it publishes one `gone` and
  forgets the token. Tracks up to 16 tokens at once.
- Publishes to `v1/{location}/{device}/{sensorId}/aux/ble/sighting`, QoS 0,
  not retained. Sightings are dropped while MQTT is offline.

```json
{"t":"f092b13c7e9cbc35","rssi":-63,"n":4}
{"t":"f092b13c7e9cbc35","gone":true}
```

Enter/leave thresholds live in the backend, in the sensor metadata
(`{"rssiEnterDbm": -75, "rssiExitDbm": -85}`), so they can be tuned without
reflashing.

## Scan timing

BLE and Wi-Fi share the ESP32 radio. The default scan is a 100 ms interval
with a 50 ms window. To hear more packets, raise the window (up to the
interval) if Wi-Fi/MQTT stays stable, by adding this line to `build_flags`
in `platformio.ini`:

```ini
    -DBLE_SCAN_WINDOW_MS=100
```

## Tests

The beacon filter and the per-token tracker are plain C++ and run on the host:

```sh
pio test -e native
```

## Testing without a phone

See the backend's BLE beacon docs: compute the current token for a beacon key
and publish a sighting with `mosquitto_pub` on the topic above.
