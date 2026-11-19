# Teruel bedrooms thermostat

## Device inventory

DS18B20 room thermostat with an active-low heating relay.

- **Firmware model** `ESP32_DS18B20_THERMOSTAT`
- **Hardware** ESP32. Custom NodeMCU-32S build. Board vendor unknown.
- **Arduino build** [`nodemcu-32s`](platformio.ini), configured version [`v1.3.4`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Habitacion | `20d74dbf-713c` | Habitaciones (thermostat) | `v1.3.4` via MQTT | `1.3.0` | `v1.3.4` | Connected |

The firmware restarts after 15 minutes without IotCloud connectivity. From 13 to 18 November, uptime telemetry showed 12 resets. One coincided with the v1.3.4 OTA on 18 November. The cause of the other resets is unconfirmed.

[Device inventory index](../README.md#device-inventory)
