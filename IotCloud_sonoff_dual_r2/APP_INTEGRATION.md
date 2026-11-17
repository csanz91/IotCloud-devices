# Sonoff Dual R2 App Integration Notes

This document describes the app-side changes needed to support the
`SONOFF_DUAL_R2` blinds controller firmware.

## Device And Sensor Identity

- Device model: `SONOFF_DUAL_R2`
- Firmware version at introduction: `v1.0.0`
- Sensor type exported by firmware: `toggle`
- Sensor id suffix used by the firmware sketch: `SonoffDualR2`
- Sensor display name used by the firmware sketch: `Persiana`

The final MQTT sensor id is built by the IotCloud firmware as:

```text
{deviceInternalId}_SonoffDualR2
```

All sensor topics therefore use:

```text
{mqtt_header}{sensor_id}/...
```

where `{mqtt_header}` is the normal device header, for example
`v1/{locationId}/{deviceId}/`.

## Command Topic

The app should publish blind movement commands to:

```text
{mqtt_header}{sensor_id}/aux/setMovement
```

Payloads are signed numeric percentages. Positive values open/up, negative
values close/down, and `0` stops. Decimals are accepted.

| App action | Payload |
| --- | --- |
| Open 10% | `10` |
| Open 25% | `25` |
| Open 50% | `50` |
| Open 75% | `75` |
| Open 100% | `100` |
| Close 10% | `-10` |
| Close 25% | `-25` |
| Close 50% | `-50` |
| Close 75% | `-75` |
| Close 100% | `-100` |
| Stop current movement | `0` |

The firmware clamps finite values outside `-100..100`. Malformed and non-finite
payloads are ignored. Non-zero movements whose computed duration is below
`250 ms` are ignored.

The firmware does not subscribe to `aux/setToggle` or the old typo endpoint
`aux/setToogle`. Use `aux/setMovement`.

## Movement Semantics

- Movement is time-based. The firmware does not track absolute blind position.
- Any movement command received while the blind is already moving stops the
  blind.
  - Example: while opening, publishing `50` stops movement.
  - Example: while opening, publishing `-25` also stops movement.
- The firmware does not reverse direction from a movement command while already
  moving. The user must send another command after the stop if they want to move
  in the opposite direction.
- After a stop, a movement in the opposite direction waits until both relays
  have been off for 500 ms (motor protection), then runs for its full
  duration. `aux/movement` reports `stopped` during the wait, and any command
  received during it cancels the queued movement.
- Movement auto-stops after the configured duration for that direction and
  percentage.
- The app should not assume a successful command means the blind reached an
  absolute position, only that the configured timed movement was requested.

## Movement Status Topic

The firmware publishes retained movement status to:

```text
{mqtt_header}{sensor_id}/aux/movement
```

Possible payloads:

| Payload | Meaning |
| --- | --- |
| `opening` | The UP relay is active |
| `closing` | The DOWN relay is active |
| `stopped` | Both relays are off |

The app should use this topic to show current movement state and to update
controls after a timed auto-stop or manual stop.

## Configurable Settings

The firmware subscribes to these setting topics:

```text
{mqtt_header}{sensor_id}/aux/fullOpenTime
{mqtt_header}{sensor_id}/aux/fullCloseTime
{mqtt_header}{sensor_id}/aux/fullOpenCommandTime
{mqtt_header}{sensor_id}/aux/disableStatusLed
```

Timing values are payloads in milliseconds. The firmware accepts complete
integer values from `1000` through `60000`, inclusive. Malformed and
out-of-range values are ignored by the firmware.

The firmware exports these settings in `sensorMetadata.settings`, so the app can
render them automatically from metadata.

Default timing values:

| Setting | Default payload | Meaning |
| --- | ---: | --- |
| `fullOpenTime` | `16667` | Opening duration used as the interpolation base for partial upward movement |
| `fullCloseTime` | `18333` | Closing duration used as the interpolation base for downward movement |
| `fullOpenCommandTime` | `23500` | Fixed duration used only for `100` upward commands |

`disableStatusLed` controls the blue status LED:

| Payload | Result |
| --- | --- |
| `true` | Disable the status LED and keep it off |
| `false` | Enable the status LED |
| Any value other than case-insensitive `true` | Enable the status LED |

Default status LED behavior is enabled.

## Suggested App UI

- Use a blinds/shutter control, not a generic binary switch.
- Show directional controls for open and close.
- Offer percentage actions such as 25%, 50%, 75%, and 100%, but publish signed
  numeric payloads so custom percentages also work.
- Include a stop action that publishes `0`.
- Subscribe to `aux/movement` to display `opening`, `closing`, or `stopped`.
- Expose advanced timing settings in milliseconds.
- Expose a boolean setting named something like "Disable status LED" mapped to
  `disableStatusLed`.

## Sensor Metadata

The firmware exports compact metadata on setup:

```json
{
  "bidirectional": true,
  "settings": [
    {
      "name": "Tiempo de apertura",
      "description": "Tiempo en milisegundos usado como base para interpolar movimientos parciales de subida.",
      "endpoint": "fullOpenTime",
      "actionType": "intpar",
      "value": 16667,
      "limits": {
        "hi": 60000,
        "lo": 1000
      }
    },
    {
      "name": "Tiempo de cierre",
      "description": "Tiempo en milisegundos usado como base para interpolar movimientos de bajada.",
      "endpoint": "fullCloseTime",
      "actionType": "intpar",
      "value": 18333,
      "limits": {
        "hi": 60000,
        "lo": 1000
      }
    },
    {
      "name": "Tiempo de apertura completa",
      "description": "Tiempo fijo en milisegundos usado solo cuando se solicita abrir al 100%.",
      "endpoint": "fullOpenCommandTime",
      "actionType": "intpar",
      "value": 23500,
      "limits": {
        "hi": 60000,
        "lo": 1000
      }
    },
    {
      "name": "Desactivar LED de estado",
      "description": "Mantiene apagado el LED azul de estado.",
      "endpoint": "disableStatusLed",
      "actionType": "boolpar",
      "value": false
    }
  ]
}
```

The app can use `sensorType == "toggle"` plus
`subType == "blinds"` to render the enhanced blinds UI.

## MQTT Examples

Open 50%:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/setMovement
Payload: 50
```

Close 100%:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/setMovement
Payload: -100
```

Stop movement:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/setMovement
Payload: 0
```

Movement status:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/movement
Payload: opening | closing | stopped
```

Set partial opening interpolation duration to 16.7 seconds:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/fullOpenTime
Payload: 16667
```

Set the fixed full-open command duration to 23.5 seconds:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/fullOpenCommandTime
Payload: 23500
```

Disable status LED:

```text
Topic:   v1/{locationId}/{deviceId}/{sensor_id}/aux/disableStatusLed
Payload: true
```

## Important Compatibility Notes

- Do not publish percentage commands to `setState`; this device uses
  `aux/setMovement`.
- Do not publish to `aux/setToggle` or `aux/setToogle`; the current firmware
  endpoint is `aux/setMovement`.
- The current firmware publishes movement state, but does not publish live
  position or a command acknowledgement for the new percentage actions.
- Do not use `stop` or `toggle` as stop. The current stop payload is numeric
  `0`.
- Settings are runtime subscriptions. Persisting user settings is an app/backend
  responsibility if values must survive firmware reboot.
