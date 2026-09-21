#include <Arduino.h>
#include <iotcloud.h>

#include "chain_waxer_sensor.h"
#include "waxer_web_portal.h"

#ifndef ONE_WIRE_BUS
// NodeMCU ESP8266 default Dallas 1-Wire pin: D1 (GPIO5)
// D2 (GPIO4), D5 (GPIO14), and D6 (GPIO12) are also fully compatible via build flags (-D ONE_WIRE_BUS=D2)
#define ONE_WIRE_BUS D1
#endif

namespace
{
Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

ChainWaxerSensor waxer_sensor("001_T", "HotPot Wax Temp", ONE_WIRE_BUS);
WaxerWebPortal web_portal(waxer_sensor);
} // namespace

void setup()
{
    Serial.begin(115200);

    Serial.println(F("========================================"));
    Serial.println(F("     Hot Pot Chain Waxer (NodeMCU)      "));
    Serial.print(F("  DS18B20 Pin: "));
#if ONE_WIRE_BUS == D1
    Serial.println(F("D1 (GPIO5)"));
#elif ONE_WIRE_BUS == D2
    Serial.println(F("D2 (GPIO4)"));
#elif ONE_WIRE_BUS == D5
    Serial.println(F("D5 (GPIO14)"));
#elif ONE_WIRE_BUS == D6
    Serial.println(F("D6 (GPIO12)"));
#else
    Serial.println(ONE_WIRE_BUS);
#endif
    Serial.println(F("========================================"));

    device.add_sensor(&waxer_sensor);

    // Initialize IoTCloud (handles WiFi, MQTT, OTA, and captive portal setup mode)
    iotcloud_setup(&device);

    // Initialize web portal & mDNS (http://chainwaxer.local)
    web_portal.begin();
}

void loop()
{
    iotcloud_loop();
    waxer_sensor.loop_fast();
    web_portal.loop();
}
