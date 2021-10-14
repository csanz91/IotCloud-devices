#include "mhz19.h"

MHZ19::MHZ19(
    const int rx,
    const int tx) : _rx(rx),
                    _tx(tx)
{
    _co2_sensor = new MHZ(rx, tx, MHZ19B);
}

void MHZ19::read_values()
{

    if (_co2_sensor->isPreHeating())
    {
        Serial.println("Preheating");
        return;
    }

    int ppm_uart = _co2_sensor->readCO2UART();
    if (ppm_uart > 0)
    {
        ppm = (float)ppm_uart;
    }
    else
    {
        return;
    }

    int temp_uart = _co2_sensor->getLastTemperature();
    if (temp_uart > 0)
    {
        temperature = (float)temp_uart;
    }
}
