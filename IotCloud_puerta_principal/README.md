# Teruel main door controller

## Device inventory

Main door actuator with a 500 ms relay pulse and a local input.

- **Firmware model** `PUERTA_PRINCIPAL`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.0.1`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Entrada | `cc50e356-892d` | Puerta (toggle/door) | `v1.0.1` via MQTT | `1.1.0` | `v1.0.1` | Connected |

The firmware restarts after 15 minutes without IotCloud connectivity.

[Device inventory index](../README.md#device-inventory)
