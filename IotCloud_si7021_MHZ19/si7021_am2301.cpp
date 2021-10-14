#include "si7021_am2301.h"

SI7021_AM2301::SI7021_AM2301(
    const int sensor_pin,
    const SensorModel sensor_model) : _sensor_pin(sensor_pin),
                                      _sensor_model(sensor_model)
{
}

bool SI7021_AM2301::waitState(int state)
{
    unsigned long timeout = micros() + 100;
    while (digitalRead(_sensor_pin) != state)
    {
        if (micros() >= timeout)
            return false;
        delayMicroseconds(1);
    }
    return true;
}

int SI7021_AM2301::read_dht_dat()
{
    byte i = 0;
    byte result = 0;
    for (i = 0; i < 8; i++)
    {
        if (!waitState(1))
            return -1;
        delayMicroseconds(35); // was 30
        if (digitalRead(_sensor_pin))
            result |= (1 << (7 - i));
        if (!waitState(0))
            return -1;
    }
    return result;
}

SI7021_AM2301::ReadingErrors SI7021_AM2301::do_plugin_read()
{
    byte i;

    pinMode(_sensor_pin, OUTPUT);
    digitalWrite(_sensor_pin, LOW); // Pull low

    switch (_sensor_model)
    {
    case SensorModel::DHT11:
        delay(19);
        break; // minimum 18ms
    case SensorModel::DHT22:
        delay(2);
        break; // minimum 1ms
    case SensorModel::DHT12:
        delay(200);
        break; // minimum 200ms
    case SensorModel::AM2301:
        delayMicroseconds(900);
        break;
    case SensorModel::SI7021:
        delayMicroseconds(500);
        break;
    }

    pinMode(_sensor_pin, INPUT_PULLUP);

    switch (_sensor_model)
    {
    case SensorModel::DHT11:
    case SensorModel::DHT22:
    case SensorModel::DHT12:
    case SensorModel::AM2301:
        delayMicroseconds(50);
        break;
    case SensorModel::SI7021:
        delayMicroseconds(20);
        break;
    }

    noInterrupts();
    if (!waitState(0))
    {
        interrupts();
        return ReadingErrors::error_no_reading;
    }
    if (!waitState(1))
    {
        interrupts();
        return ReadingErrors::error_no_reading;
    }
    if (!waitState(0))
    {
        interrupts();
        return ReadingErrors::error_no_reading;
    }

    byte dht_dat[5];
    for (i = 0; i < 5; i++)
    {
        int data = read_dht_dat();
        if (data == -1)
        {
            return ReadingErrors::error_protocol_timeout;
        }
        dht_dat[i] = data;
    }
    interrupts();

    // Checksum calculation is a Rollover Checksum by design!
    byte dht_check_sum = (dht_dat[0] + dht_dat[1] + dht_dat[2] + dht_dat[3]) & 0xFF; // check check_sum
    if (dht_dat[4] != dht_check_sum)
        return ReadingErrors::error_checksum_error;

    float read_temperature = NAN;
    float read_humidity = NAN;
    switch (_sensor_model)
    {
    case SensorModel::DHT11:
    case SensorModel::DHT12:
        read_temperature = float(dht_dat[2] * 10 + (dht_dat[3] & 0x7f)) / 10.0f; // Temperature
        if (dht_dat[3] & 0x80)
        {
            read_temperature = -read_temperature;
        }                                                            // Negative temperature
        read_humidity = float(dht_dat[0] * 10 + dht_dat[1]) / 10.0f; // Humidity
        break;
    case SensorModel::DHT22:
    case SensorModel::AM2301:
    case SensorModel::SI7021:
        if (dht_dat[2] & 0x80) // negative temperature
            read_temperature = -0.1f * word(dht_dat[2] & 0x7F, dht_dat[3]);
        else
            read_temperature = 0.1f * word(dht_dat[2], dht_dat[3]);
        read_humidity = 0.1f * word(dht_dat[0], dht_dat[1]); // Humidity
        break;
    }

    if (isnan(read_temperature) || isnan(read_humidity))
        return ReadingErrors::error_invalid_NAN_reading;

    temperature = read_temperature;
    humidity = read_humidity;

    return ReadingErrors::no_error;
}

void SI7021_AM2301::read_values()
{
    _last_error = do_plugin_read();
    switch (_last_error)
    {
    case ReadingErrors::error_no_reading:
        Serial.println(F("Error: No reading"));
        break;
    case ReadingErrors::error_protocol_timeout:
        Serial.println(F("Error: Protocol timeout"));
        break;
    case ReadingErrors::error_checksum_error:
        Serial.println(F("Error: Checksum error"));
        break;
    case ReadingErrors::error_invalid_NAN_reading:
        Serial.println(F("Error: NaN reading"));
        break;
    case ReadingErrors::no_error:
        return;
    }

    temperature = NAN;
    humidity = NAN;
}
