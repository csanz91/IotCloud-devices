# Sonoff Dual R3 blinds controller

## Device inventory

Four off-site motorized blind controllers with hold-to-move wall switches.

- **Firmware model** `SONOFF_DUAL_R3`
- **Hardware** ESP32. SONOFF Dual R3.
- **Arduino build** [`esp32dev`](platformio.ini), configured version [`v1.1.0`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Zaragoza | Comedor (sensor label) | `b8d61a62-ef50` | Comedor (toggle/blinds) | `v1.1.0` via MQTT | `1.3.0` | `v1.1.0` | Connected |
| Zaragoza | Hab. Principal (sensor label) | `b8d61a62-efa8` | Hab. Principal (toggle/blinds) | `v1.1.0` via MQTT | `1.3.0` | `v1.1.0` | Connected |
| Zaragoza | Habitación 2 (sensor label) | `b8d61a66-4d90` | Habitación 2 (toggle/blinds) | `v1.1.0` via MQTT | `1.3.0` | `v1.1.0` | Connected |
| Zaragoza | Habitación peq (sensor label) | `b8d61a66-687c` | Habitación peq (toggle/blinds) | `v1.1.0` via MQTT | `1.3.0` | `v1.1.0` | Connected |

Room labels in the table come from the DB sensor names. The registry has no separate room assignments for these four devices. At the 18 November rollout check, Habitación 2 had 43 offline episodes of at least five seconds in the preceding seven days. Forty-one started before the v1.1.0 update. All four maintained uptime through the two shared overnight disconnects after the update.

[Device inventory index](../README.md#device-inventory)

Firmware for a Sonoff Dual R3 (ESP32) driving a blind motor, with two wall
switches. Four units are installed off-site and are updated by OTA only.

- Model: `SONOFF_DUAL_R3` (must not change; the units listen on
  `v1/ota/update/SONOFF_DUAL_R3`)
- Sensor id suffix / name: `SonoffDualR3` / `Persiana` (must not change)
- Sensor type `toggle`, subtype `blinds`, same MQTT API as
  `IotCloud_sonoff_dual_r2` (see its `APP_INTEGRATION.md`)
- Partitions: default table, 1.25 MB app slots. Check the `OTA size:` line
  after every build.
- Restarts after 15 minutes without IoTCloud connectivity.

## Pins (from the `sonoff_dualr3` variant)

| Function | Constant | GPIO |
| --- | --- | --- |
| Relay up | `RELAY_1` | 27 |
| Relay down | `RELAY_2` | 14 |
| Wall switch up (active low) | `SWITCH_1` | 32 |
| Wall switch down (active low) | `SWITCH_2` | 33 |
| Reset button | `BUTTON` | 0 |
| Status LED | `LED_LINK` | 13 |

## Wall switches

The blind moves for as long as the switch is held and stops on release
(firmware ≤ v1.0.4 stopped after a fixed `actionTime` even while held).

- Press and release are accepted only after the input has been stable for
  80 ms and at least 16 consecutive samples. A sample matching the current
  state cancels a pending change, so glitches and mains hum are rejected. The
  sample count keeps that true when `loop()` runs slowly.
- For 150 ms after any relay change, input samples are ignored, because
  motor switching is what induces the noise. A release that happens in that
  window is handled right after it, not lost.
- Only a new press starts the motor. A switch already latched at boot, still
  held after a stop, or left held when the other one is released, does
  nothing.
- Both switches active stops the motor.
- A held switch is capped at 60 s.
- Reversing direction always leaves both relays off for 500 ms first.
- Oscillation guard: more than 8 switch-initiated starts within 10 s locks the
  wall switches out until both have read released for 2 s. Remote commands
  and stops still work. The state is published, retained, on
  `aux/switchLockout` (`true` / `false`). If that ever shows `true` on a
  unit, it was hitting the noise loop.

All of this lives in `src/blinds_core.cpp`, which has no Arduino dependencies.

## MQTT

| Topic (under `{mqtt_header}{sensor_id}/`) | Direction | Payload |
| --- | --- | --- |
| `aux/setMovement` | in | `-100..100`, `0` stops; same semantics as Dual R2 |
| `aux/setToggle` | in | `up` (= 100), `down` (= -100), `toggle` (= stop) |
| `aux/fullOpenTime` | in | ms, 1000–60000, default 20000 |
| `aux/fullCloseTime` | in | ms, 1000–60000, default 18333 |
| `aux/fullOpenCommandTime` | in | ms, 1000–60000, default 23500 |
| `aux/movement` | out, retained | `opening` / `closing` / `stopped` |
| `aux/switchLockout` | out, retained | `true` / `false` |

As on the Dual R2, a remote command received while moving only stops, and
timings are not persisted on the device: the backend has to republish them.

Differences from the Dual R2 `APP_INTEGRATION.md`: this device also accepts
`aux/setToggle`, publishes `aux/switchLockout`, has no status LED setting, and
defaults `fullOpenTime` to 20000. Firmware ≤ v1.0.4 used `aux/setToogle`; the
backend switches the endpoint with a per-device flag after the OTA.

## Tests

```sh
pio test -e native   # debounce, noise, feedback loop, interlock, rollover
pio run -e esp32dev  # firmware
```
