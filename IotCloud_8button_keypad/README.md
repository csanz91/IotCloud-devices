# Bedroom eight-button keypad

## Device inventory

Eight-button keypad with short and long presses and a BLE presence scanner.

- **Firmware model** `ESP32_8Button_Keypad`
- **Hardware** ESP32-C3. Custom ESP32-C3 SuperMini build. Board vendor unknown.
- **Arduino build** [`esp32-c3-supermini`](platformio.ini), configured version [`v1.1.1`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Habitación | `Bedroom Keypad Device` | Bed Beacon (ble_scanner), Bedroom Keypad (analog_subsensor) | `v1.1.1` via MQTT | `1.3.0` | `v1.1.1` | Connected |

Buttons use GPIO 7, 6, 5, 4, 0, 1, 2 and 3 in that order. OTA confirmation also checks scanner health.

[Device inventory index](../README.md#device-inventory)
