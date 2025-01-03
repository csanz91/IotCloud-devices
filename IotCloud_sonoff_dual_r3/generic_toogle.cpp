#include "generic_toogle.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
    const char *sensor_id,
    const char *sensor_name,
    const int relay1_pin,
    const int relay2_pin,
    const int switch1_pin,
    const int switch2_pin) : Toogle(sensor_id,
                                    sensor_name,
                                    "{\"bidirectional\": true, \"settings\":[{\"name\":\"Duracion\",\"description\":\"Duracion del movimiento de la persiana expresado en milisegundos.\",\"endpoint\":\"actionTime\",\"actionType\":\"intpar\",\"value\":10000,\"limits\":{\"hi\":30000,\"lo\":1000}}]}"), // 1% filtering
                             _relay1_pin(relay1_pin),
                             _relay2_pin(relay2_pin),
                             _switch1_pin(switch1_pin),
                             _switch2_pin(switch2_pin)
{

    pinMode(_relay1_pin, OUTPUT);
    pinMode(_relay2_pin, OUTPUT);
    digitalWrite(_relay1_pin, LOW);
    digitalWrite(_relay2_pin, LOW);

    pinMode(_switch1_pin, INPUT_PULLUP);
    pinMode(_switch2_pin, INPUT_PULLUP);

    _last_state_1 = digitalRead(_switch1_pin);
    _last_state_2 = digitalRead(_switch2_pin);
}

void GENERIC_TOOGLE::set_relay()
{
    if (millis() - action_start_time < action_time)
    {
        if (toogle_action == ToogleStates::UP)
        {
            digitalWrite(_relay1_pin, true);
            digitalWrite(_relay2_pin, false);
        }
        else if (toogle_action == ToogleStates::DOWN)
        {
            digitalWrite(_relay1_pin, false);
            digitalWrite(_relay2_pin, true);
        }
    }
    else if (toogle_action == ToogleStates::UP)
    {
        toogle_action = ToogleStates::INACTIVE;
    }
    else if (toogle_action == ToogleStates::DOWN)
    {
        toogle_action = ToogleStates::INACTIVE;
    }

    if (toogle_action == ToogleStates::INACTIVE)
    {
        digitalWrite(_relay1_pin, false);
        digitalWrite(_relay2_pin, false);
    }
}

void GENERIC_TOOGLE::loop()
{

    // Debounce the switch 1
    // Rest position -> active
    bool switch_state = !digitalRead(_switch1_pin);
    if (switch_state && !_last_state_1)
    {
        _mem_switch_state_1 = true;
        _last_debounce_time_1 = millis();
    }

    if ((millis() - _last_debounce_time_1) > DEBOUNCE_TIME)
    {
        if (_mem_switch_state_1 && switch_state)
        {
            _mem_switch_state_1 = false;
            _switch_activated_1 = true;
            GENERIC_TOOGLE::set_toogle_state(ToogleStates::UP);
        }
        else if (!switch_state)
        {
            _mem_switch_state_1 = false;
        }
    }

    // Active position -> rest position
    if (!switch_state && _last_state_1 && _switch_activated_1)
    {
        _mem_switch_deact_state_1 = true;
        _last_debounce_deact_time_1 = millis();
    }

    if ((millis() - _last_debounce_deact_time_1) > DEBOUNCE_TIME)
    {
        if (_mem_switch_deact_state_1 && !switch_state)
        {
            _mem_switch_deact_state_1 = false;
            _switch_activated_1 = false;
            toogle_action = ToogleStates::INACTIVE;
        }
        else if (switch_state)
        {
            _mem_switch_deact_state_1 = false;
        }
    }

    _last_state_1 = switch_state;

    // Debounce the switch 2
    // Rest position -> active
    switch_state = !digitalRead(_switch2_pin);
    if (switch_state && !_last_state_2)
    {
        _mem_switch_state_2 = true;
        _last_debounce_time_2 = millis();
    }

    if ((millis() - _last_debounce_time_2) > DEBOUNCE_TIME)
    {
        if (_mem_switch_state_2 && switch_state)
        {
            _mem_switch_state_2 = false;
            _switch_activated_2 = true;
            GENERIC_TOOGLE::set_toogle_state(ToogleStates::DOWN);
        }
        else if (!switch_state)
        {
            _mem_switch_state_2 = false;
        }
    }

    // Active position -> rest position
    if (!switch_state && _last_state_2 && _switch_activated_2)
    {
        _mem_switch_deact_state_2 = true;
        _last_debounce_deact_time_2 = millis();
    }

    if ((millis() - _last_debounce_deact_time_2) > DEBOUNCE_TIME)
    {
        if (_mem_switch_deact_state_2 && !switch_state)
        {
            _mem_switch_deact_state_2 = false;
            _switch_activated_2 = false;
            toogle_action = ToogleStates::INACTIVE;
        }
        // else if (switch_state)
        // {
        //     _mem_switch_deact_state_2 = false;
        // }
    }

    _last_state_2 = switch_state;

    set_relay();
}