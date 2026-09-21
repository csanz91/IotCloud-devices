#include <Arduino.h>
#include <iotcloud.h>

#include "motion_light_sensor.h"
#include "motion_webhook_server.h"

#define IOTCLOUD_MODEL_NUM "NODEMCU_UNIFI_MOTION_LIGHT"
#define IOTCLOUD_FW_VERSION "v1.0.0"

#ifndef UNIFI_WEBHOOK_TOKEN
#error "UNIFI_WEBHOOK_TOKEN must be injected by inject_webhook_token.py"
#endif

#ifndef MOTION_LIGHT_RELAY_ACTIVE_LOW
#define MOTION_LIGHT_RELAY_ACTIVE_LOW 0
#endif

namespace
{
constexpr uint8_t LIGHT_RELAY_PIN = D1;
constexpr uint8_t WALL_SWITCH_PIN = D2;
constexpr uint8_t FUTURE_DFPLAYER_RX_PIN = D5;
constexpr uint8_t FUTURE_DFPLAYER_TX_PIN = D6;
constexpr bool RELAY_ACTIVE_HIGH = MOTION_LIGHT_RELAY_ACTIVE_LOW == 0;

static_assert(FUTURE_DFPLAYER_RX_PIN != LIGHT_RELAY_PIN, "Reserved audio RX pin overlaps relay");
static_assert(FUTURE_DFPLAYER_TX_PIN != WALL_SWITCH_PIN, "Reserved audio TX pin overlaps switch");

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
MotionWebhookServer *webhook_server = nullptr;
} // namespace

void setup()
{
    Serial.begin(115200);

    const device_data saved_data = get_data();
    const bool runtime_enabled = validate_device_data(&saved_data);

    static MotionLightSensor motion_light(
        "MotionLight",
        "Camera Motion Light",
        LIGHT_RELAY_PIN,
        WALL_SWITCH_PIN,
        RELAY_ACTIVE_HIGH);
    static MotionWebhookServer runtime_webhook_server(
        motion_light,
        UNIFI_WEBHOOK_TOKEN,
        runtime_enabled);

    webhook_server = &runtime_webhook_server;
    device.add_sensor(&motion_light);
    iotcloud_setup(&device);
}

void loop()
{
    iotcloud_loop();
    webhook_server->loop();
}
