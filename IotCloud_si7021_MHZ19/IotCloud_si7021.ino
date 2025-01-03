#include <Arduino.h>
#include <iotcloud.h>

#include "temp_hum_sensor.h"
#include <si7021_am2301.h>
#include <co2_sensor.h>

// pin for Temp/hum reading
#define SI7021_PIN D1
// pins for CO2 reading
#define MH_Z19_RX D7 // D7
#define MH_Z19_TX D6 // D6

Device my_device = Device("v1.51", "SI7021_CO2");

SI7021_AM2301 t_h_sensor = SI7021_AM2301(SI7021_PIN, SI7021_AM2301::SensorModel::SI7021);

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", "{\"engUnits\": \"\xB0"
                                                                  "C\", \"stringFormat\": \"#0.0\",\"settings\":[{\"name\":\"Offset\",\"description\":\"Offset for the analog value.\",\"endpoint\":\"offset\",\"actionType\":\"floatpar\",\"value\":0.0, \"limits\":{\"hi\":10.0,\"lo\":-10.0}}]}",
                                          &t_h_sensor.temperature));
    my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", "{\"engUnits\": \"%\", \"stringFormat\": \"#0\",\"settings\":[{\"name\":\"Offset\",\"description\":\"Offset for the analog value.\",\"endpoint\":\"offset\",\"actionType\":\"floatpar\",\"value\":0.0, \"limits\":{\"hi\":50.0,\"lo\":-50.0}}]}", &t_h_sensor.humidity));
    my_device.add_sensor(new IotCloud_CO2("001_CO2", "CO2", "{\"engUnits\":\"ppm\",\"stringFormat\":\"#0\",\"settings\":[{\"name\":\"Calibrate\",\"description\":\"Place the sensor in an eviroment with fresh air.\",\"endpoint\":\"calibrate\",\"actionType\":\"action\",\"value\":true}]}", MH_Z19_RX, MH_Z19_TX));
    iotcloud_setup(&my_device);
}

unsigned long last_reading_T_H = 0;

void loop(void)
{
    // Read temperature and humidty every 5 seconds
    unsigned long ellapsed_since_last_read_T_H = millis() - last_reading_T_H;
    if (ellapsed_since_last_read_T_H > 5000)
    {
        t_h_sensor.read_values();
        last_reading_T_H = millis();
    }
    iotcloud_loop();
}