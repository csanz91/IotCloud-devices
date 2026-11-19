# LD2410 firmware and reporting review

All four LD2410 devices now report presence the same way. Each has a dedicated `presence` sensor built on the shared `PRESENCE_SENSOR`, which publishes a retained `<sensor>/state`. The cloud reads live state, history and daily duration for these sensors from `sensor_states`. The legacy `aux/presence` pipeline has been removed. Unifying the three light controllers into one codebase is still open and has been deferred.

State as of 19 November 2026. The ESP32 LD2410 and SONOFF Mini projects have been removed.

## What each device exposes

IDs below are sensor suffixes. Pins are listed in radar, relay, wall-switch order.

| Device | Firmware model, version | Presence | Other sensors and controls | Wiring |
| --- | --- | --- | --- | --- |
| Baño | `ESP8266_LD2410` v1.2.2 | `Presence/state` | `Switch` relay. Radar edges set the relay to match presence. Either wall-switch edge toggles it. | D1, D2, D5 |
| Cocina | `ESP8266_LD2410_TH10` v1.2.2 | `KitchenPresence/state` | `KitchenLight` relay. Either wall-switch edge toggles it. | GPIO2, GPIO12, GPIO14 |
| Salon Central | `ESP8266_LD2410_SALON_CENTRAL` v1.0.3 | `Presence/state` | `Switch` relay. Either wall-switch edge toggles it. | D6, D2, D1 |
| Salon (light unit) | `ESP8266_LD2410_LIGHT` v1.2.0 | `004_P/state` | `003_L` BH1750 lux. Sensor only: no relay, no notifier. | D1, none, none. BH1750 on D3/D4. |

[Baño](../IotCloud_ld2410/src/IotCloud_ld2410.cpp#L20) · [Cocina](../IotCloud_ld2410_TH10/src/IotCloud_ld2410_TH10.cpp#L20) · [Salon Central](../IotCloud_ld2410_salon_central/src/IotCloud_ld2410_salon_central.cpp#L20) · [Salon light unit](../IotCloud_ld2410_light/src/IotCloud_ld2410_light.cpp) · [Shared presence sensor](../../IotCloud-sensors/src/presence_sensor.cpp#L17)

The shared presence sensor publishes a retained value after a five-second startup delay, on every debounced change and on every reconnect, and it retries failed publishes.

## Salon light unit migration (19 November 2026)

1. **v1.1.0.** Added the dedicated `004_P` sensor. Removed the D2 relay drive, the lux gate and the `001_N` notifier. Kept `002_S/aux/presence` as a temporary mirror.
2. **Database.**
   - Registered `bcddc224-879f_004_P`, copying the room, color, order and per-user settings from `002_S`.
   - Copied the 92,543 historical `002_S` presence rows into `sensor_states` under `004_P`.
   - Deleted the `001_N` and `002_S` sensor rows.
   - Backup: `backups/2026-11-19_ld2410_presence_migration/`, which is not committed.
3. **Backend (IotCloud2 `97bdd0c`).**
   - The gateway no longer subscribes to, stores or caches `aux/presence`.
   - The API's presence history and daily duration read `sensor_states`.
   - The snapshot `presence` field and `bool_field` were removed.
   - The automation runner and the lan_adapter Otter monitor use `004_P`.
   - The `sensor_presence` table is kept for historical data, but nothing writes to it or reads it any more.
4. **v1.2.0.** Removed the legacy `002_S` mirror. OTA finished successfully and fresh `004_P` and `003_L` data was confirmed afterwards.

IotCloudWeb is deprecated and was not deployed. Its local working tree still has uncommitted changes that ignore `aux/presence`.

## Remaining items

- **Stale retained broker topics.** These are harmless because nothing reads them, but they should be cleared:
  - `2cf4327a-732c_Switch/aux/presence`
  - `bcddc224-879f_002_S/aux/presence`
  - `bcddc224-879f_002_S/state`
  - `bcddc224-879f_002_S/aux/disableNotifications`

  To clear one, run `emqx ctl retainer clean "<full topic>"` in `iotcloud2-emqx-1`, or publish an empty retained payload to it.
- **Despacho `2cf43279-f068_002_S`** has old `sensor_presence` rows (Dec 2024 to Jan 2025). They overlap its `sensor_states` rows and were not merged.
- **Unifying the light controllers (deferred).** Cocina and Salon Central have identical `generic_switch.{h,cpp}`. Baño is the same controller with "radar drives relay" on top. One project with three PlatformIO envs can serve all three if it keeps each device's pins, sensor IDs, `model_num` and flash layout. Cocina is a 1 MB ESP8285 TH10, so check the OTA image fits.
