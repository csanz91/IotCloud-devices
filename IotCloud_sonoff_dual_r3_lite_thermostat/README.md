# IotCloud_sonoff_dual_r3_lite_thermostat

Thermostat on a modified Sonoff Dual R3 Lite (ESP32), with a DS18B20 probe.

## Hardware mods

- Relay 1 removed (GPIO27 is just driven low).
- Relay 2 (GPIO14) wired as a **dry contact**: OUT1 and OUT2 close when
  heating is on. It is the heating output.
- Inputs S1/S2 (GPIO32/33) are broken and not used.

## Wiring the DS18B20

| DS18B20 | Board |
|---------|-------|
| VDD (red) | 3V3 |
| GND (black) | GND |
| DATA (yellow) | GPIO17 (the pad the Madrid presence sensor uses) |

Add a 4.7 k resistor between DATA and 3V3. Use a sealed (stainless/waterproof)
probe and put it **outside** the enclosure: the ESP32, power supply and relay
heat the inside and skew the reading.

**Mains warning:** the Dual R3 Lite's low-voltage GND is probably not isolated
from mains. Treat the sensor lead as live, use an insulated probe, and never
connect a USB-serial adapter while the board is on mains.

To change the pin, edit `ONE_WIRE_BUS` in
`src/IotCloud_sonoff_dual_r3_lite_thermostat.cpp`.

## Behaviour

- Same thermostat logic as `IotCloud_thermostat` (setpoint and hysteresis come
  from the cloud), but the relay is **active-high**, as on all Sonoff relays.
- Heating is off at boot. If the connection is lost while heating it is
  switched off after 30 minutes, and the device restarts after 15 minutes
  without connectivity.
- Default partitions (OTA slot 1.25 MB; the image is about 1.23 MB).

## Build and flash

```sh
pio run -e esp32dev -t upload
```

Then provision with the usual IotCloud setup flow.
