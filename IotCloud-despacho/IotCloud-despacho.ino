#include <Arduino.h>
#include <iotcloud.h>

#include "hexa_led.h"
#include "presence_sensor.h"
#include "co2_sensor.h"
#include "temp_hum_sensor.h"

#define PRESENCE_PIN D2
// pins for CO2 reading
#define MH_Z19_RX D7  // D7
#define MH_Z19_TX D6  // D6
// pin for Temp/hum reading
#define SI7021_PIN D1

Device my_device = Device("v1.0.2", "Despacho");
SI7021_AM2301 t_h_sensor = SI7021_AM2301(SI7021_PIN, SI7021_AM2301::SensorModel::SI7021);


void setup(void) {
  Serial.begin(115200);
  my_device.add_sensor(new HexaLed("001", "Hexaled"));
  my_device.add_sensor(new PRESENCE_SENSOR("002_S", "Presence", PRESENCE_PIN));
  my_device.add_sensor(new IotCloud_CO2("001_CO2", "CO2", "", MH_Z19_RX, MH_Z19_TX));
  my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", "", &t_h_sensor.temperature));
  my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", "", &t_h_sensor.humidity));

  iotcloud_setup(&my_device);
}

unsigned long last_reading_T_H = 0;

void loop(void) {
  // Read temperature and humidty every 25 seconds
  unsigned long ellapsed_since_last_read_T_H = millis() - last_reading_T_H;
  if (ellapsed_since_last_read_T_H > 25000) {
    t_h_sensor.read_values();
    last_reading_T_H = millis();
  }
  iotcloud_loop();
}