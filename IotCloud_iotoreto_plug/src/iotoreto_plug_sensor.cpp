#include "iotoreto_plug_sensor.h"

#include <Arduino.h>

namespace
{
const char PLUG_METADATA[] =
    "{\"engUnits\":\"W\",\"stringFormat\":\"#0\",\"settings\":[{\"name\":\"Disable Status LED\",\"description\":\"Keep the status LED off.\",\"endpoint\":\"disableStatusLed\",\"actionType\":\"boolpar\",\"value\":false}]}";

const uint32_t RTC_STATE_MAGIC = 0x49504C47; // "IPLG"
const uint32_t RTC_STATE_OFFSET = 64;

struct RtcRelayState
{
    uint32_t magic;
    uint32_t crc;
    uint32_t relay_on;
    uint32_t writes;
};

uint32_t rtc_crc(const RtcRelayState &state)
{
    return state.magic ^ state.relay_on ^ state.writes ^ 0xA5A55A5A;
}
}

IotoretoPlugSensor::IotoretoPlugSensor(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin,
    const int switch_pin,
    const int led_pin)
    : BaseSensor(sensor_id, sensor_name, "smart_plug", PLUG_METADATA),
      _relay_pin(relay_pin),
      _switch_pin(switch_pin),
      _led_pin(led_pin),
      _rx_buf{0},
      _rx_count(0),
      _state(false),
      _status_led_enabled(true),
      _state_restored_from_rtc(false),
      _last_button_state(HIGH),
      _button_was_pressed(false),
      _last_debounce_time(0),
      _last_report_ms(0),
      _last_health_report_ms(0),
      _voltage(0),
      _current(0),
      _power(0),
      _energy_kwh(0),
      _last_frame_ms(0)
{
    _state_restored_from_rtc = restore_state();

    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, _state ? HIGH : LOW);

    if (_led_pin != -1) {
        pinMode(_led_pin, OUTPUT);
        set_status_led(_state);
    }

    pinMode(_switch_pin, INPUT_PULLUP);
    _last_button_state = digitalRead(_switch_pin);
}

void IotoretoPlugSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);
    subscribe_topics(mqtt_client);

    // CSE7766 uses GPIO3 (hardware UART RX) at 4800 baud, even parity.
    Serial.flush();
    Serial.begin(4800, SERIAL_8E1);

    report_state();
    report_power();
    report_health();
}

void IotoretoPlugSensor::subscribe_topics(EspMQTTClient *mqtt_client)
{
    char topic[128] = "";

    construct_topic(topic, sizeof(topic), "setState");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        set_state(payload.equalsIgnoreCase("true"));
    });

    construct_topic(topic, sizeof(topic), "aux/disableStatusLed");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        _status_led_enabled = !payload.equalsIgnoreCase("true");
        set_status_led(_state);
    });

    construct_topic(topic, sizeof(topic), "aux/reset_energy");
    mqtt_client->subscribe(topic, [&](const String &) {
        _energy_kwh = 0.0f;
        report_power();
    });
}

void IotoretoPlugSensor::set_state(bool state)
{
    if (_state == state) {
        report_state();
        return;
    }

    _state = state;
    digitalWrite(_relay_pin, _state ? HIGH : LOW);
    persist_state();
    set_status_led(_state);
    report_state();
}

void IotoretoPlugSensor::set_status_led(bool on)
{
    if (_led_pin == -1) {
        return;
    }

    // Athom Plug V2 LED (GPIO13) is active-LOW: LOW = on, HIGH = off.
    digitalWrite(_led_pin, (_status_led_enabled && on) ? LOW : HIGH);
}

void IotoretoPlugSensor::report_state()
{
    if (!_mqtt_client || !_mqtt_client->isConnected()) {
        return;
    }

    char topic[128] = "";
    construct_topic(topic, sizeof(topic), "state");
    _mqtt_client->publish(topic, _state ? "true" : "false", true);
}

void IotoretoPlugSensor::report_aux_metric(const char *endpoint, float value, unsigned short decimals)
{
    char topic[128] = "";
    char payload[16];

    construct_topic(topic, sizeof(topic), endpoint);
    dtostrf(value, 1, decimals, payload);
    _mqtt_client->publish(topic, payload, true);
}

void IotoretoPlugSensor::report_power()
{
    if (!_mqtt_client || !_mqtt_client->isConnected()) {
        return;
    }

    char topic[128] = "";
    char payload[16];

    construct_topic(topic, sizeof(topic), "value");
    dtostrf(_power, 1, 0, payload);
    _mqtt_client->publish(topic, payload, true);

    report_aux_metric("aux/voltage", _voltage, 1);
    report_aux_metric("aux/current", _current, 3);
    report_aux_metric("aux/energy", _energy_kwh, 4);
}

void IotoretoPlugSensor::report_health()
{
    if (!_mqtt_client || !_mqtt_client->isConnected()) {
        return;
    }

    char topic[128] = "";
    char payload[16];

    construct_topic(topic, sizeof(topic), "aux/uptime_s");
    ultoa(millis() / 1000UL, payload, 10);
    _mqtt_client->publish(topic, payload, true);

    construct_topic(topic, sizeof(topic), "aux/free_heap");
    ultoa(ESP.getFreeHeap(), payload, 10);
    _mqtt_client->publish(topic, payload, true);

    construct_topic(topic, sizeof(topic), "aux/relay_restored_from_rtc");
    _mqtt_client->publish(topic, _state_restored_from_rtc ? "true" : "false", true);

    construct_topic(topic, sizeof(topic), "aux/meter_last_frame_age_s");
    if (_last_frame_ms == 0) {
        _mqtt_client->publish(topic, "-1", true);
    } else {
        ultoa((millis() - _last_frame_ms) / 1000UL, payload, 10);
        _mqtt_client->publish(topic, payload, true);
    }
}

bool IotoretoPlugSensor::restore_state()
{
    RtcRelayState rtc_state;

    if (!ESP.rtcUserMemoryRead(RTC_STATE_OFFSET, reinterpret_cast<uint32_t *>(&rtc_state), sizeof(rtc_state))) {
        return false;
    }

    if (rtc_state.magic != RTC_STATE_MAGIC || rtc_state.crc != rtc_crc(rtc_state)) {
        return false;
    }

    _state = rtc_state.relay_on != 0;
    return true;
}

void IotoretoPlugSensor::persist_state()
{
    RtcRelayState rtc_state;

    if (!ESP.rtcUserMemoryRead(RTC_STATE_OFFSET, reinterpret_cast<uint32_t *>(&rtc_state), sizeof(rtc_state)) ||
        rtc_state.magic != RTC_STATE_MAGIC ||
        rtc_state.crc != rtc_crc(rtc_state)) {
        rtc_state.magic = RTC_STATE_MAGIC;
        rtc_state.writes = 0;
    }

    rtc_state.relay_on = _state ? 1 : 0;
    rtc_state.writes++;
    rtc_state.crc = rtc_crc(rtc_state);

    ESP.rtcUserMemoryWrite(RTC_STATE_OFFSET, reinterpret_cast<uint32_t *>(&rtc_state), sizeof(rtc_state));
}

bool IotoretoPlugSensor::is_frame_valid(const uint8_t *f)
{
    if (f[1] != FRAME_SYNC) {
        return false;
    }

    uint8_t checksum = 0;
    for (int i = 2; i < 23; i++) {
        checksum += f[i];
    }
    return checksum == f[23];
}

void IotoretoPlugSensor::resync()
{
    // The buffer didn't hold a valid frame: keep the bytes from the next
    // "<header> 0x5A" pair on, so a frame that started mid-buffer isn't lost.
    for (uint8_t start = 1; start + 1 < FRAME_LEN; start++) {
        if (_rx_buf[start + 1] == FRAME_SYNC) {
            _rx_count = FRAME_LEN - start;
            memmove(_rx_buf, _rx_buf + start, _rx_count);
            return;
        }
    }

    _rx_buf[0] = _rx_buf[FRAME_LEN - 1];
    _rx_count = 1;
}

bool IotoretoPlugSensor::parse_frame()
{
    const uint8_t *f = _rx_buf;
    const uint8_t header = f[0];

    // Header 0x55 is a normal frame. 0xFC-0xFF report a chip fault, and 0xAA
    // means the chip is not calibrated, so its coefficients can't be used.
    // Other 0xFx headers flag a measurement that is out of range: with no
    // load the chip sends 0xF2 ("power cycle exceeds range"), meaning 0 W.
    if ((header & 0xFC) == 0xFC || header == 0xAA) {
        return false;
    }
    if (header != 0x55 && (header & 0xF0) != 0xF0) {
        return false;
    }

    const uint32_t v_coef = ((uint32_t)f[2] << 16) | ((uint32_t)f[3] << 8) | f[4];
    const uint32_t v_data = ((uint32_t)f[5] << 16) | ((uint32_t)f[6] << 8) | f[7];
    const uint32_t i_coef = ((uint32_t)f[8] << 16) | ((uint32_t)f[9] << 8) | f[10];
    const uint32_t i_data = ((uint32_t)f[11] << 16) | ((uint32_t)f[12] << 8) | f[13];
    const uint32_t p_coef = ((uint32_t)f[14] << 16) | ((uint32_t)f[15] << 8) | f[16];
    const uint32_t p_data = ((uint32_t)f[17] << 16) | ((uint32_t)f[18] << 8) | f[19];

    const uint8_t validity = f[20];
    _voltage = ((validity & 0x40) && v_data > 0) ? (float)v_coef / v_data : 0.0f;
    _current = ((validity & 0x20) && i_data > 0) ? (float)i_coef / i_data : 0.0f;
    const bool power_out_of_range = (header & 0xF2) == 0xF2;
    _power = ((validity & 0x10) && p_data > 0 && !power_out_of_range) ? (float)p_coef / p_data : 0.0f;
    if (_power == 0.0f) {
        _current = 0.0f;
    }

    // Only count energy across a short gap between frames. After a long gap
    // (lost frames, a stalled loop) the power in between is unknown.
    const unsigned long now_ms = millis();
    if (_last_frame_ms > 0 && now_ms - _last_frame_ms <= MAX_ENERGY_GAP_MS) {
        _energy_kwh += _power * (float)(now_ms - _last_frame_ms) / 3600000000.0f;
    }
    _last_frame_ms = now_ms;

    return true;
}

void IotoretoPlugSensor::read_meter()
{
    uint8_t bytes_read = 0;

    while (Serial.available() && bytes_read < MAX_SERIAL_BYTES_PER_LOOP) {
        const uint8_t b = (uint8_t)Serial.read();
        bytes_read++;

        // Frames are "<header> 0x5A <22 bytes>". The header byte varies, so
        // sync on the 0x5A that follows it; any byte may start a frame.
        if (_rx_count == 1 && b != FRAME_SYNC) {
            _rx_buf[0] = b;
            continue;
        }

        _rx_buf[_rx_count++] = b;

        if (_rx_count == FRAME_LEN) {
            if (is_frame_valid(_rx_buf)) {
                parse_frame();
                _rx_count = 0;
            } else {
                resync();
            }
        }
    }

    if (bytes_read > 0) {
        delay(0);
    }
}

void IotoretoPlugSensor::handle_button()
{
    const bool reading = digitalRead(_switch_pin);
    const unsigned long now = millis();

    if (reading != _last_button_state) {
        _last_debounce_time = now;
        _last_button_state = reading;
        return;
    }

    if (now - _last_debounce_time < DEBOUNCE_DELAY_MS) {
        return;
    }

    const bool pressed = reading == LOW;

    if (pressed && !_button_was_pressed) {
        _button_was_pressed = true;
        return;
    }

    if (!pressed && _button_was_pressed) {
        _button_was_pressed = false;
        set_state(!_state);
    }
}

void IotoretoPlugSensor::loop()
{
    read_meter();
    handle_button();

    const unsigned long now = millis();
    if (now - _last_report_ms >= REPORT_PERIOD_MS) {
        report_power();
        _last_report_ms = now;
    }

    if (now - _last_health_report_ms >= HEALTH_REPORT_PERIOD_MS) {
        report_health();
        _last_health_report_ms = now;
    }
}
