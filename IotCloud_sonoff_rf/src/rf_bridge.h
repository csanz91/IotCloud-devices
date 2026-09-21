#pragma once
#include <sensors/sensor.h>
#include "rf_protocol.h"

class RfBridge : public BaseSensor {
    static constexpr size_t MAX_DEVICES = 5;
    static constexpr size_t MAX_CANDIDATES = 8;
    struct Entry {
        char id[33] = {}, message[97] = {};
        rf::Signal signal;
        uint32_t debounce = 3000;
        uint8_t repeats = 2;
        rf::Filter filter;
    };
    struct Candidate { rf::Signal signal; uint16_t count = 0; };
    Entry entries_[MAX_DEVICES];
    Candidate candidates_[MAX_CANDIDATES];
    size_t count_ = 0, candidateCount_ = 0;
    rf::Parser parser_;
    bool mounted_ = false, healthy_ = false, learning_ = false;
    uint32_t learnStart_ = 0;
    rf::Signal transmittedSignal_;
    bool suppressTransmitEcho_ = false;
    uint32_t transmittedAt_ = 0;
    uint32_t received_ = 0, unknown_ = 0, rejected_ = 0, published_ = 0;
    uint32_t transmitted_ = 0, suppressed_ = 0;
    char bootId_[17] = {};
    char learningSessionId_[65] = {};
    uint32_t eventSequence_ = 0, registryRevision_ = 0, snapshotSequence_ = 0;
    void sendCode(const rf::Signal &signal);
    void publish(const char *endpoint, JsonDocument &doc, bool retained = false);
    void status();
    void result(const char *operation, const char *state, const char *requestId = nullptr,
                const char *delivery = nullptr);
    void list();
    void command(const char *operation, const String &payload);
    void receive(const rf::Signal &signal, uint32_t now);
    bool load();
    bool save();
    static bool decode(JsonVariantConst json, Entry &entry);
    static void encode(JsonObject json, const Entry &entry);
public:
    RfBridge() : BaseSensor("001_RF", "RF bridge", "rf_bridge", "{}", "v2.0.0") {}
    void begin();
    void init(char *header, EspMQTTClient *client) override;
    void poll();
};
