#include <Arduino.h>
#include <iotcloud.h>

#include "co2_sensor.h"
#include "mhz19.h"

// pin for uart reading
#define MH_Z19_RX D7 // D7
#define MH_Z19_TX D6 // D6

Device my_device = Device("v1.0", "CO2");

MHZ19 sensor = MHZ19(MH_Z19_RX, MH_Z19_TX);

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IotCloud_CO2_T("001_T", "Temperature", "{\"engUnits\": \"\xB0"
                                                           "C\", \"stringFormat\": \"#0.0\"}",
                                   &sensor.temperature));
    my_device.add_sensor(new IotCloud_CO2_T("001_CO2", "CO2", "{\"engUnits\": \"ppm\", \"stringFormat\": \"#0\"}", &sensor.ppm));
    iotcloud_setup(&my_device);
}

unsigned long last_reading = 0;

void loop(void)
{
    unsigned long ellapsed_since_last_read = millis() - last_reading;
    if (ellapsed_since_last_read > 120000)
    {
        sensor.read_values();
        last_reading = millis();
    }
    iotcloud_loop();
}