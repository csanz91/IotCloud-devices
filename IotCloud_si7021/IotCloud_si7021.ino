#include <Arduino.h>
#include <iotcloud.h>

#include "temp_hum_sensor.h"
#include "si7021_am2301.h"

#define PIN D1

Device my_device = Device("v1.0", "SI7021");

SI7021_AM2301 sensor = SI7021_AM2301(PIN, SI7021_AM2301::SensorModel::SI7021);

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", "{\"engUnits\": \"\xB0""C\", \"stringFormat\": \"#0.0\"}", &sensor.temperature));
    my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", "{\"engUnits\": \"%\", \"stringFormat\": \"#0\"}", &sensor.humidity));
    iotcloud_setup(&my_device);
}

unsigned long last_reading = 0;

void loop(void)
{
    unsigned long ellapsed_since_last_read = millis() - last_reading;
    if (ellapsed_since_last_read > 5000)
    {
        sensor.read_values();
        last_reading = millis();
    }
    iotcloud_loop();
}