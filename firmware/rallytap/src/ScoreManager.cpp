#include "ScoreManager.h"

ScoreManager::ScoreManager()
    : _a(0)
    , _b(0)
    , _set_a(0)
    , _set_b(0)
    , _status("ok")
    , _msg("")
    , _leftName("")
    , _rightName("")
{}

bool ScoreManager::fromJSON(const String& json) {
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return false;
    }

    // New rallytap.* envelopes nest the score payload under "score"; the
    // legacy Phase-1 payload carries the fields at the top level. Accept both.
    JsonVariant score = doc["score"];
    const bool nested = !score.isNull();

    _a      = nested ? doc["score"]["a"]     | 0 : doc["a"]     | 0;
    _b      = nested ? doc["score"]["b"]     | 0 : doc["b"]     | 0;
    _set_a  = nested ? doc["score"]["set_a"] | 0 : doc["set_a"] | 0;
    _set_b  = nested ? doc["score"]["set_b"] | 0 : doc["set_b"] | 0;
    _status = nested ? doc["score"]["status"] | "ok" : doc["status"] | "ok";
    _msg    = nested ? doc["score"]["msg"]    | ""  : doc["msg"]    | "";

    _leftName  = doc["leftName"]  | "";
    _rightName = doc["rightName"] | "";

    return true;
}

String ScoreManager::toJSON() const {
    StaticJsonDocument<512> doc;

    doc["a"]     = _a;
    doc["b"]     = _b;
    doc["set_a"] = _set_a;
    doc["set_b"] = _set_b;
    doc["status"] = _status;
    doc["msg"]    = _msg;
    doc["leftName"]  = _leftName;
    doc["rightName"] = _rightName;

    String output;
    serializeJson(doc, output);
    return output;
}

String ScoreManager::formatDisplay() const {
    // Two lines, side-bound (FW-2/A3/E10/CONF-3): names come pre-swapped from
    // the hub and are rendered exactly as received — no swap logic on the tap.
    String left  = _leftName.length()  > 0 ? _leftName  + " " : "";
    String right = _rightName.length() > 0 ? _rightName + " " : "";
    return left  + String(_a) + "\n" + right + String(_b);
}
