#include <Arduino.h>
#include <iotcloud.h>
#include "iotoreto_plug_sensor.h"

// Athom Plug V2 — ESP8285H16 2 MB
// Tasmota template identifies GPIO3 as CSE7766 UART RX (4800 baud, even parity).
// The board is sometimes listed as HLW8032-compatible, but the observed frames are CSE7766.
//   GPIO5  — Button (INPUT_PULLUP, active LOW)
//   GPIO12 — Relay (active HIGH)
//   GPIO13 — Status LED (active LOW / inverted)
#define RELAY_PIN   12
#define SWITCH_PIN   5
#define LED_PIN     13

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

void setup(void)
{
    my_device.add_sensor(new IotoretoPlugSensor("IotoretoPlug", "Smart Plug", RELAY_PIN, SWITCH_PIN, LED_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}
