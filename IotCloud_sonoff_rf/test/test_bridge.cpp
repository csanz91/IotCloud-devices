#include "../src/rf_bridge.h"
#include <LittleFS.h>
#include <cassert>
#include <iostream>

static char header[] = "v1/location/bridge/";
static String rfTopic(const char *endpoint) {
    return String(header) + "internal_001_RF/aux/rf/" + endpoint;
}
static JsonDocument latest(EspMQTTClient &mqtt, const char *endpoint) {
    for (auto it = mqtt.messages.rbegin(); it != mqtt.messages.rend(); ++it) {
        if (it->topic == rfTopic(endpoint)) {
            JsonDocument doc; assert(!deserializeJson(doc, it->payload)); return doc;
        }
    }
    assert(false); return JsonDocument{};
}
static String learningSession;
static unsigned requestCounter = 0;
static void command(EspMQTTClient &mqtt, const char *op, const char *fields = "") {
    String payload = "{";
    const String requestId = "request-" + std::to_string(++requestCounter);
    payload += "\"request_id\":\"" + requestId + "\"";
    if ((!strcmp(op, "register") || !strcmp(op, "cancel")) && !learningSession.empty())
        payload += ",\"learning_session_id\":\"" + learningSession + "\"";
    if (*fields == ',') ++fields;
    if (*fields) payload += String(",") + fields;
    payload += "}";
    mqtt.callbacks.at(rfTopic(op))(payload);
    if (!strcmp(op, "learn") && latest(mqtt, "result")["result"] == "learning")
        learningSession = requestId;
    if ((!strcmp(op, "register") && latest(mqtt, "result")["result"] == "registered") ||
        (!strcmp(op, "cancel") && latest(mqtt, "result")["result"] == "cancelled"))
        learningSession.clear();
}
static void expect(EspMQTTClient &mqtt, const char *result) {
    assert(latest(mqtt, "result")["result"] == result);
}
static void frame(RfBridge &bridge, uint32_t code, uint16_t low = 300) {
    const uint8_t bytes[] = {0xAA,0xA4,0x20,0x00,uint8_t(low>>8),uint8_t(low),0x03,0x84,
                            uint8_t(code>>16),uint8_t(code>>8),uint8_t(code),0x55};
    Serial.input.insert(Serial.input.end(), std::begin(bytes), std::end(bytes));
    testNow += 100; bridge.poll();
}
static size_t uartFrames(const std::vector<uint8_t> &bytes, uint8_t wanted) {
    size_t at = 0, found = 0;
    while (at < bytes.size()) {
        assert(bytes[at] == 0xAA && at + 2 < bytes.size());
        const uint8_t action = bytes[at + 1];
        const size_t length = action == 0xA5 ? 12 : 3;
        assert(at + length <= bytes.size() && bytes[at + length - 1] == 0x55);
        if (action == wanted) ++found;
        at += length;
    }
    return found;
}
int main() {
    RfBridge bridge; EspMQTTClient mqtt;
    bridge.begin(); bridge.init(header, &mqtt);
    assert(latest(mqtt,"status")["devices"] == 0);
    auto statusCount = [&]() { size_t n=0; for (auto &m:mqtt.messages) if(m.topic==rfTopic("status")) ++n; return n; };
    assert(statusCount() == 1);
    const auto committed = "/rf-devices.json";
    // Unknown traffic cannot enroll devices before learning starts.
    frame(bridge, 55);
    assert(!LittleFS.exists(committed));
    command(mqtt,"learn");
    assert(statusCount() == 2);
    assert(latest(mqtt,"status")["protocol_version"] == 1);
    assert(latest(mqtt,"status")["boot_id"].as<const char *>()[0]);
    assert(latest(mqtt,"status")["learning_session_id"] == learningSession);
    const size_t beforeRejectedSend = uartFrames(Serial.output, 0xA5);
    command(mqtt,"send",",\"sync\":8192,\"low\":300,\"high\":900,\"code\":1906529");
    expect(mqtt,"learning_active");
    assert(uartFrames(Serial.output, 0xA5) == beforeRejectedSend);
    frame(bridge,1906529);
    command(mqtt,"register",",\"id\":\"door\",\"code\":1906529");
    expect(mqtt,"candidate_needs_three_readings");
    frame(bridge,1906529); frame(bridge,1906529);
    command(mqtt,"register",",\"id\":\"door\",\"code\":1906529,\"message\":\"Door open\"");
    expect(mqtt,"registered");
    assert(statusCount() == 3);
    assert(LittleFS.exists(committed));
    const String saved = *LittleFS.files.at(committed);
    // A successful send writes the stock A5 frame, while an immediate matching A4 self-echo is suppressed.
    Serial.output.clear();
    const size_t statusBeforeSend = statusCount();
    command(mqtt,"send",",\"sync\":8192,\"low\":300,\"high\":900,\"code\":1906529");
    expect(mqtt,"sent");
    assert(statusCount() == statusBeforeSend); // Send does not trigger duplicate status publish
    assert(latest(mqtt,"result")["delivery"] == "rf_mcu_uart");
    assert(latest(mqtt,"result")["receiver_confirmed"] == false);
    assert((Serial.output == std::vector<uint8_t>{0xAA,0xA5,0x20,0x00,0x01,0x2C,0x03,0x84,0x1D,0x17,0x61,0x55}));
    // Per-device repeats; unrelated code and mismatched timings never trigger it.
    auto eventCount = [&]() { size_t n=0; for (auto &m:mqtt.messages) if(m.topic==rfTopic("event")) ++n; return n; };
    frame(bridge,1906529);
    assert(eventCount()==0);
    testNow += 1001;
    frame(bridge, 1906529); frame(bridge, 123); frame(bridge,1906529,800);
    assert(eventCount()==0);
    frame(bridge,1906529); assert(eventCount()==1);
    assert(latest(mqtt,"event")["sequence"] == 1);
    assert(latest(mqtt,"event")["protocol_version"] == 1);
    assert(latest(mqtt,"event")["boot_id"] == latest(mqtt,"status")["boot_id"]);
    assert(latest(mqtt,"event")["occurred_at"].is<const char *>());
    const std::string occurredAt = latest(mqtt,"event")["occurred_at"].as<std::string>();
    assert(occurredAt.length() == 20 && occurredAt.back() == 'Z');
    for (auto &m:mqtt.messages) if(m.topic==rfTopic("event")) assert(!m.retained);
    for (auto &m:mqtt.messages) assert(m.topic.find("aux/notification") == String::npos);
    // Failed writes/renames keep both RAM and the committed registry unchanged.
    command(mqtt,"learn");
    for(int i=0;i<3;++i) frame(bridge,456);
    LittleFS.failWrite=true;
    command(mqtt,"register",",\"id\":\"second\",\"code\":456"); expect(mqtt,"storage_error");
    LittleFS.failWrite=false; LittleFS.failRename=true;
    command(mqtt,"register",",\"id\":\"second\",\"code\":456"); expect(mqtt,"storage_error");
    assert(*LittleFS.files.at(committed)==saved);
    assert(latest(mqtt,"status")["devices"]==1);
    command(mqtt,"delete",",\"id\":\"door\""); expect(mqtt,"storage_error");
    assert(latest(mqtt,"status")["devices"]==1);
    LittleFS.failRename=false;
    command(mqtt,"register",",\"id\":\"second\",\"code\":456"); expect(mqtt,"registered");
    // Full reboot reloads committed entries, ignoring an orphan temporary file.
    RfBridge reboot; EspMQTTClient after;
    reboot.begin(); reboot.init(header,&after);
    assert(latest(after,"status")["devices"]==2);
    command(after,"learn");
    for(int i=0;i<3;++i) frame(reboot,456);
    command(after,"register",",\"id\":\"duplicate\",\"code\":456"); expect(after,"code_already_registered");
    command(after,"register",",\"id\":\"bad/topic\",\"code\":456"); expect(after,"invalid_id");
    testNow+=60000; reboot.poll();
    command(after,"register",",\"id\":\"second\",\"code\":456"); expect(after,"no_learning_session");
    command(after,"delete",",\"id\":\"door\""); expect(after,"deleted");
    assert(latest(after,"status")["devices"]==1);
    // Reconnect cancels active learning.
    command(after,"learn");
    reboot.init(header,&after);
    command(after,"register",",\"id\":\"second\",\"code\":456");
    expect(after,"no_learning_session");
    // Corruption is reported; it is never silently formatted or overwritten.
    *LittleFS.files.at(committed)="broken";
    RfBridge corrupt; EspMQTTClient corruptMqtt;
    corrupt.begin(); corrupt.init(header,&corruptMqtt);
    assert(latest(corruptMqtt,"status")["storage"]=="invalid_registry");
    command(corruptMqtt,"learn"); expect(corruptMqtt,"storage_error");
    assert(*LittleFS.files.at(committed)=="broken");
    command(corruptMqtt,"initialize"); expect(corruptMqtt,"confirmation_required");
    command(corruptMqtt,"initialize",",\"confirm\":\"ERASE_RF_REGISTRY\""); expect(corruptMqtt,"initialized");
    assert(!LittleFS.exists(committed));
    assert(uartFrames(Serial.output, 0xA0) > 0);
    std::cout << "RF registration/storage/MQTT tests passed\n";
}
