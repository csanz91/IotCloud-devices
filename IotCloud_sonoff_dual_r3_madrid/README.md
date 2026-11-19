# Madrid SONOFF Dual R3 lights and presence

## Device inventory

Two room light relays, a GPIO17 presence input and a passive BLE scanner.

- **Firmware model** `SONOFF_DUAL_R3_MADRID`
- **Hardware** ESP32. SONOFF Dual R3 family.
- **Arduino build** [`esp32dev`](platformio.ini), configured version [`v1.2.1`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Despacho, Habitación | `2415d6d6-cdc0` | Despacho (light/light_switch), Habitacion (light/light_switch), Habitacion Beacon (ble_scanner), Pres. Habitación (presence) | `v1.2.1` via MQTT | `1.3.0` | `v1.2.1` | Connected |

The unit is installed in a wall. It confirms a new OTA image after MQTT and the BLE scanner pass the health check.

[Device inventory index](../README.md#device-inventory)
