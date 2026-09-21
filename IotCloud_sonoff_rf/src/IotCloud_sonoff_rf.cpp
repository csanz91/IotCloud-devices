#include <Arduino.h>
#include <iotcloud.h>
#include "rf_bridge.h"

Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
RfBridge bridge;

void setup() {
    Serial.begin(19200, SERIAL_8N1);
    Serial.setDebugOutput(false);
    bridge.begin();
    my_device.add_sensor(&bridge);
    iotcloud_setup(&my_device);
}

void loop() {
    // Drain RF even in provisioning mode; BaseSensor::loop is unused.
    bridge.poll();
    iotcloud_loop();
}
