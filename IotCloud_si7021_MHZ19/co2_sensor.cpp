#include "co2_sensor.h" x

IotCloud_CO2::IotCloud_CO2(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata,
    const int rx, const int tx) : AnalogSensor(sensor_id,
                                               sensor_name,
                                               30000,
                                               1,
                                               true,
                                               "analog",
                                               metadata,
                                               0.03) // 3% filtering
{
    _serial = new SoftwareSerial(rx, tx);
    _serial->begin(9600);
    _co2_sensor = new MHZ19();
    _co2_sensor->begin(*_serial);
    _co2_sensor->autoCalibration(false);
    _co2_sensor->setFilter(true, true);
}

void IotCloud_CO2::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    AnalogSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/calibrate");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               Serial.println("Calibration requested...");
                               _co2_sensor->calibrate();
                           });
}

void IotCloud_CO2::get_value()
{
    int ppm_uart = _co2_sensor->getCO2(true);
    if (ppm_uart != 0)
    {
        float ppm = (float)ppm_uart;
        set_value(ppm);
    }
}
