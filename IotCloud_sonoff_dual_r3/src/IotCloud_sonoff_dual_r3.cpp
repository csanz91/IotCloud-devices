#include <Arduino.h>
#include <iotcloud.h>

#include "sonoff_dual_r3_blinds.h"

// Pins come from the sonoff_dualr3 board variant:
// RELAY_1 = GPIO27 (up), RELAY_2 = GPIO14 (down),
// SWITCH_1 = GPIO32 (up), SWITCH_2 = GPIO33 (down).
static constexpr unsigned long OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

void setup(void)
{
    Serial.begin(115200);

    // Sensor id and name must not change: the backend knows these blinds by them.
    my_device.add_sensor(new SonoffDualR3Blinds(
        "SonoffDualR3",
        "Persiana",
        RELAY_1,
        RELAY_2,
        SWITCH_1,
        SWITCH_2));

    iotcloud_setup(&my_device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop(void)
{
    iotcloud_loop();
}
