#include "rf_bridge.h"
#include <LittleFS.h>
#include <memory>
#include <time.h>

namespace {
constexpr const char *REGISTRY = "/rf-devices.json";
constexpr const char *TEMP = "/rf-devices.tmp";
constexpr uint32_t LEARN_MS = 60000;
constexpr uint32_t TRANSMIT_ECHO_MS = 1000;
void signalJson(JsonObject json, const rf::Signal &signal) {
    json["code"] = signal.code;
    json["sync"] = signal.sync;
    json["low"] = signal.low;
    json["high"] = signal.high;
}
bool validId(const char *id) {
    if (!id || !*id || strlen(id) > 32) return false;
    for (; *id; ++id)
        if (!((*id >= 'a' && *id <= 'z') || (*id >= 'A' && *id <= 'Z') ||
              (*id >= '0' && *id <= '9') || *id == '_' || *id == '-')) return false;
    return true;
}
}

void RfBridge::sendCode(const rf::Signal &signal) {
    const uint8_t command[] = {
        0xAA, 0xA5,
        uint8_t(signal.sync >> 8), uint8_t(signal.sync),
        uint8_t(signal.low >> 8), uint8_t(signal.low),
        uint8_t(signal.high >> 8), uint8_t(signal.high),
        uint8_t(signal.code >> 16), uint8_t(signal.code >> 8), uint8_t(signal.code),
        0x55
    };
    transmittedSignal_ = signal;
    transmittedAt_ = millis();
    suppressTransmitEcho_ = true;
    Serial.write(command, sizeof(command));
    Serial.flush();
    ++transmitted_;
}
void RfBridge::begin() {
#ifdef ARDUINO
    snprintf(bootId_, sizeof(bootId_), "%08lx%08lx",
             (unsigned long)ESP.random(), (unsigned long)ESP.random());
#else
    static uint32_t bootCounter = 0;
    snprintf(bootId_, sizeof(bootId_), "host%012lu", (unsigned long)++bootCounter);
#endif
    LittleFSConfig config;
    config.setAutoFormat(false);
    LittleFS.setConfig(config);
    mounted_ = LittleFS.begin();
    healthy_ = mounted_ && load();
}
void RfBridge::init(char *header, EspMQTTClient *client) {
    BaseSensor::init(header, client);
    learning_ = false;
    for (const char *op : {"learn", "register", "delete", "cancel", "list", "initialize", "send"}) {
        char topic[160];
        const String endpoint = String("aux/rf/") + op;
        construct_topic(topic, sizeof(topic), endpoint.c_str());
        client->subscribe(topic, [this, op](const String &payload) { command(op, payload); });
    }
    status();
    list();
}
void RfBridge::publish(const char *endpoint, JsonDocument &doc, bool retained) {
    if (!_mqtt_client || !_mqtt_client->isConnected()) return;
    String payload;
    serializeJson(doc, payload);
    char topic[160];
    const String path = String("aux/rf/") + endpoint;
    construct_topic(topic, sizeof(topic), path.c_str());
    _mqtt_client->publish(topic, payload, retained);
}
void RfBridge::status() {
    JsonDocument doc;
    doc["protocol_version"] = 1;
    doc["boot_id"] = bootId_;
    doc["registry_revision"] = registryRevision_;
    doc["storage"] = !mounted_ ? "unmounted" : healthy_ ? "ready" : "invalid_registry";
    doc["devices"] = count_;
    doc["learning"] = learning_;
    if (learning_) doc["learning_session_id"] = learningSessionId_;
    doc["received"] = received_;
    doc["unknown"] = unknown_;
    doc["rejected"] = rejected_;
    doc["published"] = published_;
    doc["transmitted"] = transmitted_;
    doc["suppressed"] = suppressed_;
    doc["parser_errors"] = parser_.errors;
    publish("status", doc, true);
}
void RfBridge::result(const char *operation, const char *state, const char *requestId,
                      const char *delivery) {
    JsonDocument doc;
    doc["protocol_version"] = 1;
    doc["boot_id"] = bootId_;
    if (requestId) doc["request_id"] = requestId;
    doc["operation"] = operation;
    doc["result"] = state;
    if (delivery) {
        doc["delivery"] = delivery;
        doc["receiver_confirmed"] = false;
    }
    publish("result", doc);
}
void RfBridge::encode(JsonObject json, const Entry &entry) {
    json["id"] = entry.id;
    json["message"] = entry.message;
    json["repeats"] = entry.repeats;
    json["debounce_ms"] = entry.debounce;
    signalJson(json, entry.signal);
}
bool RfBridge::decode(JsonVariantConst json, Entry &entry) {
    const char *id = json["id"].as<const char *>();
    const char *message = json["message"].as<const char *>();
    if (!json.is<JsonObjectConst>() || !validId(id) || !message || strlen(message) > 96 ||
        !json["code"].is<uint32_t>() || json["code"].as<uint32_t>() > 0xFFFFFF ||
        !json["sync"].is<uint16_t>() || !json["low"].is<uint16_t>() || !json["high"].is<uint16_t>() ||
        !json["repeats"].is<uint8_t>() || !json["debounce_ms"].is<uint32_t>()) return false;
    entry = Entry{};
    strcpy(entry.id, id);
    strcpy(entry.message, message);
    entry.signal.code = json["code"];
    entry.signal.sync = json["sync"];
    entry.signal.low = json["low"];
    entry.signal.high = json["high"];
    entry.repeats = json["repeats"];
    entry.debounce = json["debounce_ms"];
    return entry.signal.valid() && entry.repeats >= 1 && entry.repeats <= 5 &&
           entry.debounce >= 100 && entry.debounce <= 60000;
}
bool RfBridge::load() {
    if (!LittleFS.exists(REGISTRY)) {
        // An interrupted first save is not a committed registry.
        count_ = 0;
        return true;
    }
    File file = LittleFS.open(REGISTRY, "r");
    if (!file || file.size() > 12288) return false;
    JsonDocument doc;
    if (deserializeJson(doc, file) || doc["version"] != 1 || !doc["devices"].is<JsonArray>()) return false;
    JsonArray array = doc["devices"];
    if (array.size() > MAX_DEVICES) return false;
    std::unique_ptr<Entry[]> loaded(new Entry[MAX_DEVICES]);
    size_t n = 0;
    for (JsonVariant item : array) {
        if (!decode(item, loaded[n])) return false;
        for (size_t i = 0; i < n; ++i)
            if (!strcmp(loaded[i].id, loaded[n].id) || loaded[i].signal.code == loaded[n].signal.code) return false;
        ++n;
    }
    for (size_t i = 0; i < n; ++i) entries_[i] = loaded[i];
    count_ = n;
    return true;
}
bool RfBridge::save() {
    if (!mounted_ || !healthy_) return false;
    JsonDocument doc;
    doc["version"] = 1;
    JsonArray array = doc["devices"].to<JsonArray>();
    for (size_t i = 0; i < count_; ++i) encode(array.add<JsonObject>(), entries_[i]);
    if (doc.overflowed()) return false;
    File file = LittleFS.open(TEMP, "w");
    if (!file) return false;
    const size_t expected = measureJson(doc);
    const size_t written = serializeJson(doc, file);
    file.flush();
    file.close();
    if (written != expected) return false;
    file = LittleFS.open(TEMP, "r");
    JsonDocument verify;
    if (!file || file.size() != expected || deserializeJson(verify, file) || verify != doc) return false;
    file.close();
    // LittleFS atomic replacement: never remove the committed file first.
    return LittleFS.rename(TEMP, REGISTRY);
}
void RfBridge::list() {
    char snapshotId[33];
    snprintf(snapshotId, sizeof(snapshotId), "%s-%lu", bootId_,
             (unsigned long)++snapshotSequence_);
    // Individual records stay below IoTCloud's 1024-byte MQTT packet limit.
    for (size_t i = 0; i < count_; ++i) {
        JsonDocument doc;
        doc["protocol_version"] = 1;
        doc["boot_id"] = bootId_;
        doc["registry_revision"] = registryRevision_;
        doc["snapshot_id"] = snapshotId;
        doc["index"] = i;
        doc["total"] = count_;
        encode(doc["device"].to<JsonObject>(), entries_[i]);
        publish("devices", doc);
    }
    JsonDocument done;
    done["protocol_version"] = 1;
    done["boot_id"] = bootId_;
    done["registry_revision"] = registryRevision_;
    done["snapshot_id"] = snapshotId;
    done["total"] = count_;
    done["complete"] = true;
    publish("devices", done);
}
void RfBridge::command(const char *op, const String &payload) {
    JsonDocument doc;
    if (payload.length() > 768 || deserializeJson(doc, payload) || !doc.is<JsonObject>()) {
        result(op, "invalid_json"); return;
    }
    const char *requestId = doc["request_id"].as<const char *>();
    if (!requestId || !*requestId || strlen(requestId) > 64) {
        result(op, "invalid_request_id"); return;
    }
    if (!strcmp(op, "list")) { list(); result(op, "listed", requestId); return; }
    if (!strcmp(op, "initialize")) {
        if (doc["confirm"] != "ERASE_RF_REGISTRY") { result(op, "confirmation_required", requestId); return; }
        learning_ = false;
        LittleFS.end();
        mounted_ = false;
        healthy_ = false;
        count_ = 0;
        if (LittleFS.format()) mounted_ = LittleFS.begin();
        healthy_ = mounted_;
        if (healthy_) ++registryRevision_;
        status();
        result(op, healthy_ ? "initialized" : "storage_error", requestId); list(); return;
    }
    if (!strcmp(op, "send")) {
        if (learning_) { result(op, "learning_active", requestId); return; }
        if (!doc["sync"].is<uint16_t>() || !doc["low"].is<uint16_t>() ||
            !doc["high"].is<uint16_t>() || !doc["code"].is<uint32_t>()) {
            result(op, "invalid_signal", requestId); return;
        }
        rf::Signal signal;
        signal.sync = doc["sync"];
        signal.low = doc["low"];
        signal.high = doc["high"];
        signal.code = doc["code"];
        if (!signal.valid()) { result(op, "invalid_signal", requestId); return; }
        sendCode(signal);
        result(op, "sent", requestId, "rf_mcu_uart"); return;
    }
    if (!healthy_) { result(op, "storage_error", requestId); return; }
    if (!strcmp(op, "learn")) {
        if (learning_) { result(op, "learning_active", requestId); return; }
        candidateCount_ = 0;
        learning_ = true;
        strncpy(learningSessionId_, requestId, sizeof(learningSessionId_) - 1);
        learnStart_ = millis();
        status();
        result(op, "learning", requestId); return;
    }
    if (!strcmp(op, "cancel")) {
        const char *sessionId = doc["learning_session_id"].as<const char *>();
        if (!learning_ || !sessionId || strcmp(sessionId, learningSessionId_)) {
            result(op, "invalid_learning_session", requestId); return;
        }
        learning_ = false;
        status();
        result(op, "cancelled", requestId); return;
    }
    const char *id = doc["id"].as<const char *>();
    if (!validId(id)) { result(op, "invalid_id", requestId); return; }
    size_t index = count_;
    for (size_t i = 0; i < count_; ++i) if (!strcmp(id, entries_[i].id)) index = i;
    if (!strcmp(op, "delete")) {
        if (index == count_) { result(op, "not_found", requestId); return; }
        Entry previous = entries_[index];
        entries_[index] = entries_[count_ - 1];
        --count_;
        if (!save()) {
            ++count_; entries_[index] = previous;
            result(op, "storage_error", requestId); return;
        }
        ++registryRevision_;
        status();
        result(op, "deleted", requestId); list(); return;
    }
    if (!learning_ || uint32_t(millis() - learnStart_) >= LEARN_MS) {
        if (learning_) { learning_ = false; status(); }
        result(op, "no_learning_session", requestId); return;
    }
    const char *sessionId = doc["learning_session_id"].as<const char *>();
    if (!sessionId || strcmp(sessionId, learningSessionId_)) {
        result(op, "invalid_learning_session", requestId); return;
    }
    if (!doc["code"].is<uint32_t>()) { result(op, "invalid_code", requestId); return; }
    const uint32_t code = doc["code"];
    const Candidate *selected = nullptr;
    for (size_t i = 0; i < candidateCount_; ++i)
        if (candidates_[i].signal.code == code && candidates_[i].count >= 3) selected = &candidates_[i];
    if (!selected) { result(op, "candidate_needs_three_readings", requestId); return; }
    for (size_t i = 0; i < count_; ++i)
        if (i != index && entries_[i].signal.code == code) { result(op, "code_already_registered", requestId); return; }
    if (index == MAX_DEVICES) { result(op, "registry_full", requestId); return; }
    signalJson(doc.as<JsonObject>(), selected->signal);
    if (doc["message"].isUnbound()) doc["message"] = id;
    if (doc["repeats"].isUnbound()) doc["repeats"] = 2;
    if (doc["debounce_ms"].isUnbound()) doc["debounce_ms"] = 3000;
    Entry entry;
    if (!decode(doc.as<JsonVariantConst>(), entry)) { result(op, "invalid_registration", requestId); return; }
    Entry previous = entries_[index];
    const size_t oldCount = count_;
    entries_[index] = entry;
    if (index == count_) ++count_;
    if (!save()) {
        count_ = oldCount; entries_[index] = previous;
        result(op, "storage_error", requestId); return;
    }
    learning_ = false;
    ++registryRevision_;
    status();
    result(op, "registered", requestId); list();
}
void RfBridge::receive(const rf::Signal &signal, uint32_t now) {
    ++received_;
    if (!signal.valid()) { ++rejected_; return; }
    if (suppressTransmitEcho_) {
        if (uint32_t(now - transmittedAt_) <= TRANSMIT_ECHO_MS && rf::matches(signal, transmittedSignal_)) {
            ++suppressed_;
            return;
        }
        if (uint32_t(now - transmittedAt_) > TRANSMIT_ECHO_MS) suppressTransmitEcho_ = false;
    }
    if (learning_) {
        size_t index = candidateCount_;
        for (size_t i = 0; i < candidateCount_; ++i)
            if (candidates_[i].signal.code == signal.code) index = i;
        if (index < MAX_CANDIDATES) {
            if (index == candidateCount_) {
                candidates_[index] = Candidate{};
                candidates_[index].signal = signal;
                ++candidateCount_;
            }
            auto &candidate = candidates_[index];
            if (rf::matches(signal, candidate.signal)) {
                if (candidate.count < 65535) ++candidate.count;
                // Bound MQTT output even if a nearby transmitter repeats continuously.
                if (candidate.count <= 3) {
                    JsonDocument doc;
                    doc["protocol_version"] = 1;
                    doc["boot_id"] = bootId_;
                    doc["learning_session_id"] = learningSessionId_;
                    signalJson(doc.to<JsonObject>(), candidate.signal);
                    doc["readings"] = candidate.count;
                    doc["ready"] = candidate.count >= 3;
                    publish("candidate", doc);
                }
            }
        }
    }
    for (size_t i = 0; i < count_; ++i) {
        Entry &entry = entries_[i];
        if (signal.code != entry.signal.code) continue;
        if (!rf::matches(signal, entry.signal)) { ++rejected_; return; }
        if (!entry.filter.accept(now, entry.repeats, entry.debounce)) return;
        if (!_mqtt_client || !_mqtt_client->isConnected()) return;
        JsonDocument doc;
        doc["protocol_version"] = 1;
        doc["boot_id"] = bootId_;
        doc["sequence"] = ++eventSequence_;
        doc["id"] = entry.id;
        doc["message"] = entry.message;
        doc["uptime_ms"] = now;
        const time_t rawTime = time(nullptr);
        struct tm tmInfo;
        if (rawTime > 1000000000 && gmtime_r(&rawTime, &tmInfo)) {
            char occurredAt[25];
            strftime(occurredAt, sizeof(occurredAt), "%Y-%m-%dT%H:%M:%SZ", &tmInfo);
            doc["occurred_at"] = occurredAt;
        } else {
            doc["occurred_at"] = nullptr;
        }
        signalJson(doc.as<JsonObject>(), signal);
        publish("event", doc);
        ++published_;
        return;
    }
    ++unknown_;
}
void RfBridge::poll() {
    const uint32_t now = millis();
    if (learning_ && (!_mqtt_client || !_mqtt_client->isConnected() || uint32_t(now - learnStart_) >= LEARN_MS)) {
        learning_ = false;
        status();
        result("learn", "expired", learningSessionId_);
    }
    rf::Frame frame;
    size_t budget = 128;
    while (budget-- && Serial.available()) {
        if (!parser_.feed(uint8_t(Serial.read()), millis(), frame)) continue;
        if (frame.action == 0xA2 || frame.action == 0xA3 || frame.action == 0xA4) {
            const uint8_t ack[] = {0xAA, 0xA0, 0x55};
            Serial.write(ack, sizeof(ack));
        }
        // Registration listens to ordinary A4 traffic; A3 is never auto-enrolled.
        if (frame.action == 0xA4) receive(frame.signal, millis());
    }
}
