# Sonoff Dual R3 Lite — BLE presence scanner

## Device inventory

Two light relays and a passive BLE presence scanner for an entryway.

- **Firmware model** `SONOFF_DUAL_R3_LITE_BLE`
- **Hardware** ESP32. SONOFF Dual R3 Lite.
- **Arduino build** [`esp32dev`](platformio.ini), configured version [`v1.1.0`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 09:01 CET.

No matching database record or MQTT model announcement was found. The OTA server has `v1.1.0`. The release table lists `v1.2.0`, for which no image was found in the model directory.

The existing installation notes refer to an entryway unit. Its current database name and location could not be verified.

[Device inventory index](../README.md#device-inventory)

ESP32 firmware for a Sonoff Dual R3 Lite entryway scanner. It
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

The firmware uses the `min_spiffs.csv` partition table (two 1.9 MB app slots).
v1.0.0 used NimBLE, which didn't fit the default 1.25 MB slots. A partition
table can't change over OTA, so flash over serial the first time; later
updates can use OTA.

```sh
pio run -e esp32dev -t upload
```

Then provision with the usual IotCloud setup flow.

## What the scanner does

- Continuous passive scan, duplicate filtering off, no results stored
  (phones randomize their MAC address). The code is in
  `shared/libs/IotCloudBleScanner`. It sends HCI commands straight to the BT
  controller (VHCI), with no BLE host stack, which saves about 27 KB of flash.
- If no advertising at all is heard for 60 s, it re-runs the HCI init.
  Every 60 s it logs tracked tokens, dropped samples and minimum free heap.
- A new OTA image stays pending until the scan has run with MQTT connected
  for 60 s. If it reboots before that, the bootloader goes back to the
  previous image.
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

The beacon filter, the HCI/advertising parser and the per-token tracker are
plain C++ and run on the host:

```sh
pio test -e native
```

## Testing without a phone

See the backend's BLE beacon docs: compute the current token for a beacon key
and publish a sighting with `mosquitto_pub` on the topic above.
