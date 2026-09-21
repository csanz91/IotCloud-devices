#include <Arduino.h>
#include <iotcloud.h>
#include "sonoff_dual_r2_blinds.h"

static constexpr int RELAY_UP_PIN = 12;
static constexpr int RELAY_DOWN_PIN = 5;
static constexpr int STATUS_LED_PIN = 13;
static constexpr unsigned long OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new SonoffDualR2Blinds(
        "SonoffDualR2",
        "Persiana",
        RELAY_UP_PIN,
        RELAY_DOWN_PIN,
        STATUS_LED_PIN));

    iotcloud_setup(&my_device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop(void)
{
    iotcloud_loop();
}
