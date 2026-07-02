#include "ScoreManager.h"

ScoreManager::ScoreManager()
    : _a(0)
    , _b(0)
    , _set_a(0)
    , _set_b(0)
    , _status("ok")
    , _msg("")
{}

bool ScoreManager::fromJSON(const String& json) {
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return false;
    }

    _a     = doc["a"]     | 0;
    _b     = doc["b"]     | 0;
    _set_a = doc["set_a"] | 0;
    _set_b = doc["set_b"] | 0;
    _status = doc["status"] | "ok";
    _msg    = doc["msg"]    | "";

    return true;
}

String ScoreManager::toJSON() const {
    StaticJsonDocument<128> doc;

    doc["a"]     = _a;
    doc["b"]     = _b;
    doc["set_a"] = _set_a;
    doc["set_b"] = _set_b;
    doc["status"] = _status;
    doc["msg"]    = _msg;

    String output;
    serializeJson(doc, output);
    return output;
}

String ScoreManager::formatDisplay() const {
    return "A:" + String(_a) + " B:" + String(_b);
}
