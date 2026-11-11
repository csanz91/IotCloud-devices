# ArduinoOTA / ESPmDNS stubs

No-op stand-ins for the core's `ArduinoOTA` and `ESPmDNS` libraries.

EspMQTTClient links both for its LAN OTA and web updater, but IotCloud never
turns those on (it updates over HTTPS from the OTA server). Replacing them
saves about 56 KB of flash, which lets the BLE scanner fit in the default
1.25 MB app slot of devices that can only be updated over OTA.

Opt in per project, never in `common_env.ini`:

```ini
lib_extra_dirs =
    ${esp32_base.lib_extra_dirs}
    ../shared/no_ota_stubs
```

Only the calls EspMQTTClient makes are stubbed. If firmware starts using
mDNS or ArduinoOTA for real, drop this directory from that project.
After a build, `libespressif__mdns.a` should not appear in `firmware.map`.
