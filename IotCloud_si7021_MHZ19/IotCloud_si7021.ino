#include <Arduino.h>
#include <iotcloud.h>

#include "temp_hum_sensor.h"
#include "si7021_am2301.h"
#include "co2_sensor.h"
#include "mhz19.h"

// pin for Temp/hum reading
#define SI7021_PIN D1
// pins for CO2 reading
#define MH_Z19_RX D7 // D7
#define MH_Z19_TX D6 // D6

Device my_device = Device("v1.0", "SI7021_CO2");

SI7021_AM2301 t_h_sensor = SI7021_AM2301(SI7021_PIN, SI7021_AM2301::SensorModel::SI7021);
MHZ19 CO2_sensor = MHZ19(MH_Z19_RX, MH_Z19_TX);

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", "{\"engUnits\": \"\xB0"
                                                                  "C\", \"stringFormat\": \"#0.0\"}",
                                          &t_h_sensor.temperature));
    my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", "{\"engUnits\": \"%\", \"stringFormat\": \"#0\"}", &t_h_sensor.humidity));
    my_device.add_sensor(new IotCloud_CO2_T("001_CO2", "CO2", "{\"engUnits\": \"ppm\", \"stringFormat\": \"#0\"}", &CO2_sensor.ppm));
    iotcloud_setup(&my_device);
}

unsigned long last_reading_T_H = 0;
unsigned long last_reading_CO2 = 0;

void loop(void)
{
    // Read temperature and humidty every 5 seconds
    unsigned long ellapsed_since_last_read_T_H = millis() - last_reading_T_H;
    if (ellapsed_since_last_read_T_H > 5000)
    {
        t_h_sensor.read_values();
        last_reading_T_H = millis();
    }
    // Get CO2 every 60 seconds. The sensor only provides a value every 120 seconds
    unsigned long ellapsed_since_last_read_CO2 = millis() - last_reading_CO2;
    if (ellapsed_since_last_read_CO2 > 60000)
    {
        CO2_sensor.read_values();
        last_reading_CO2 = millis();
    }
    iotcloud_loop();
}