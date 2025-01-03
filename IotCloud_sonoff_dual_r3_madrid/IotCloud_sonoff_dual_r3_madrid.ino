#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"

Device my_device = Device("v1.0.0", "SONOFF_DUAL_R3_MADRID");

// static const uint8_t TX = 1;
// static const uint8_t RX = 3;

// static const uint8_t BUTTON = 0;
// static const uint8_t LED_LINK = 13;
// static const uint8_t RELAY_2 = 14;
// static const uint8_t RELAY_1 = 27;
// static const uint8_t SWITCH_2 = 33;
// static const uint8_t SWITCH_1 = 32;

// static const uint8_t CSE7761_TX = 25;
// static const uint8_t CSE7761_RX = 26;

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("SonoffDualR3", "Switch1", RELAY_1, SWITCH_1));
    my_device.add_sensor(new GENERIC_TOOGLE("SonoffDualR3", "Switch2", RELAY_2, SWITCH_2));
    iotcloud_setup(&my_device);

    pinMode(CSE7761_TX, INPUT_PULLUP);
    pinMode(CSE7761_RX, INPUT_PULLUP);
}

void loop(void)
{
    iotcloud_loop();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 2000)
    {                                         // Check every 2 seconds
        int state1 = digitalRead(CSE7761_TX); // SWITCH_1
        int state2 = digitalRead(CSE7761_RX); // SWITCH_2
        Serial.printf("Switch states: SW1=%d SW2=%d\n", state1, state2);
        lastPrint = millis();
    }
}