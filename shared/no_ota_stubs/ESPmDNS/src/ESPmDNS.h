#pragma once
// No-op stand-in. See shared/no_ota_stubs/README.md.
#include <stdint.h>

class MDNSResponder
{
public:
    bool begin(const char *) { return false; }
    void end() {}
    bool addService(const char *, const char *, uint16_t) { return false; }
};

inline MDNSResponder MDNS;
