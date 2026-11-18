#include <Arduino.h>
#include <iotcloud.h>
#include <ble_scanner_sensor.h>

#include "generic_switch.h"

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
static BleScannerSensor ble_scanner("001_BLE", "BLE presence");

static constexpr uint32_t OTA_CONFIRM_AFTER_MS = 60000;

void setup(void)
{
    Serial.begin(115200);

    // Pins come from the sonoff_dualr3 board variant.
    my_device.add_sensor(new GENERIC_TOOGLE("Switch1", "Switch1", RELAY_1, SWITCH_1));
    my_device.add_sensor(new GENERIC_TOOGLE("Switch2", "Switch2", RELAY_2, SWITCH_2));
    my_device.add_sensor(&ble_scanner);

    // Scan only once provisioned; setup/AP mode doesn't need the radio shared.
    const device_data saved_data = get_data();
    if (validate_device_data(&saved_data))
    {
        ble_scanner.begin_scanning();
    }

    // The library keeps a new OTA image pending until MQTT is up; also wait
    // for the scanner, so an image that breaks BLE rolls back too.
    OTA::set_health_check([] { return ble_scanner.healthy_for(OTA_CONFIRM_AFTER_MS); });
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}
