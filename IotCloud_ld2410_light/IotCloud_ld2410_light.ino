#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"
#include "light_sensor.h"
// #include "time.h"
// #include "TZ.h"

#define RELAY_PIN D2
#define SWITCH_PIN D1


BH1750 lightMeter;
float lux;

Device my_device = Device("v1.0.2", "ESP8266_LD2410_LIGHT");

NOTIFIER *notifier = new NOTIFIER("001_N", "Notifier", "{}");

// bool getLocalTime(struct tm *info)
// {
//   time_t now;
//   time(&now);
//   struct tm *timeinfo = localtime(&now);

//   return false;
// }

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("002_S", "Switch", RELAY_PIN, SWITCH_PIN, notifier, &lux));
    my_device.add_sensor(new LIGHT_SENSOR("003_L", "Light", "{\"engUnits\": \"lux\", \"stringFormat\": \"#0.0\"}", &lux));
    my_device.add_sensor(notifier);
    iotcloud_setup(&my_device);

    // Required to reset the sensor after an OTA update
    digitalWrite(D3, LOW);
    digitalWrite(D4, LOW);

    Wire.begin(D3, D4); // sda (blue), scl (green)
    lightMeter.begin();
}

// void printLocalTime()
// {
//   time_t now;
//   struct tm *timeinfo;
//   time(&now);
//   timeinfo = localtime(&now);

//   char buffer[80];
//   strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
//   Serial.println(buffer);
// }

void loop(void)
{
    iotcloud_loop();

    static unsigned long last_luminosity_report_time = 0;
    const unsigned long now = millis();

    // Report luminosity every 5 seconds
    if (now - last_luminosity_report_time >= 5000)
    {
        lux = lightMeter.readLightLevel();
        Serial.print("Light: ");
        Serial.println(lux);
        
        // // configTzTime(TZ_Europe_Madrid, IotCloud_Constants::NTP_SERVER);

        // printLocalTime();

        last_luminosity_report_time = now;
    }
}