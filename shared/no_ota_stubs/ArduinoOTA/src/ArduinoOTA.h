#pragma once
// No-op stand-in. See shared/no_ota_stubs/README.md.
#include <stdint.h>

class ArduinoOTAClass
{
public:
    ArduinoOTAClass &setHostname(const char *) { return *this; }
    ArduinoOTAClass &setPassword(const char *) { return *this; }
    ArduinoOTAClass &setPort(uint16_t) { return *this; }
    void begin() {}
    void handle() {}
};

inline ArduinoOTAClass ArduinoOTA;
