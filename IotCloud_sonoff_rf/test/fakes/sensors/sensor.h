#pragma once
#include <ArduinoJson.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <deque>
#include <cstdio>
using String = std::string;
inline uint32_t testNow = 0;
inline uint32_t millis() { return testNow; }
struct FakeSerial {
    std::deque<uint8_t> input;
    std::vector<uint8_t> output;
    size_t available() { return input.size(); }
    int read() { auto b = input.front(); input.pop_front(); return b; }
    void write(const uint8_t *b, size_t n) { output.insert(output.end(), b, b+n); }
    void flush() {}
};
inline FakeSerial Serial;
struct EspMQTTClient {
    struct Message { String topic, payload; bool retained; };
    bool connected = true;
    std::vector<Message> messages;
    std::map<String, std::function<void(const String &)>> callbacks;
    bool isConnected() { return connected; }
    bool publish(const String &topic, const String &payload, bool retained = false) {
        messages.push_back({topic, payload, retained}); return connected;
    }
    void subscribe(const String &topic, std::function<void(const String &)> callback) { callbacks[topic] = callback; }
};
class BaseSensor {
protected:
    char *_mqtt_header = nullptr;
    EspMQTTClient *_mqtt_client = nullptr;
public:
    BaseSensor(const char *, const char *, const char *, const char *, const char *) {}
    virtual void init(char *header, EspMQTTClient *client) { _mqtt_header = header; _mqtt_client = client; }
    void construct_topic(char *out, size_t n, const char *endpoint) {
        snprintf(out, n, "%sinternal_001_RF/%s", _mqtt_header, endpoint);
    }
};
