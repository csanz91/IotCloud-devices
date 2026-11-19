# Madrid heating relay

## Device inventory

Heating thermostat with an active-high relay on D2 (GPIO4). It receives temperature from a separate subsensor.

- **Firmware model** `MADRID_SUBSENSOR_THERMOSTAT`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.0.4`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Salon | `cc50e30a-1d4c` | Termostato (thermostat) | `v1.0.4` via MQTT | `1.3.0` | `v1.0.4` | Connected |

The [Madrid subsensor](../IotCloud_thermostat_subsensor/README.md) publishes readings to this thermostat. The firmware restarts after 15 minutes without IotCloud connectivity. At the 18 November rollout check, its preceding seven days contained ten offline episodes of at least five seconds. The longest was about 33 minutes.

[Device inventory index](../README.md#device-inventory)
