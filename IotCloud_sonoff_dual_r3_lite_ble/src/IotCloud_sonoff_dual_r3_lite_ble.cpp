#include <Arduino.h>
#include <iotcloud.h>

#include "ble_scanner_sensor.h"
#include "generic_switch.h"

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

void setup(void)
{
    Serial.begin(115200);

    // Pins come from the sonoff_dualr3 board variant.
    my_device.add_sensor(new GENERIC_TOOGLE("Switch1", "Switch1", RELAY_1, SWITCH_1));
    my_device.add_sensor(new GENERIC_TOOGLE("Switch2", "Switch2", RELAY_2, SWITCH_2));

    static BleScannerSensor ble_scanner("001_BLE", "BLE presence");
    my_device.add_sensor(&ble_scanner);

    // Scan only once provisioned; setup/AP mode doesn't need the radio shared.
    const device_data saved_data = get_data();
    if (validate_device_data(&saved_data))
    {
        ble_scanner.begin_scanning();
    }

    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}
