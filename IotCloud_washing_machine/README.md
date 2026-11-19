# Washing machine notifier and start button

## Device inventory

Watches the washing-machine LED to detect completion and uses a servo to press the start button.

- **Firmware model** `WASHING_MACHINE`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.1.3`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Unassigned | `bcddc224-98c8` | Wachine Machine (notifier/appliance), Wachine Machine (toggle/appliance) | `v1.1.3` in DB | `1.1.1` | `v1.1.3` | Offline. Latest DB event 2026-11-09. |

Both database entries use the original label `Wachine Machine`, one for the notifier and one for the actuator.

[Device inventory index](../README.md#device-inventory)
