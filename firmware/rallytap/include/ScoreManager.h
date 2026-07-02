#ifndef SCORE_MANAGER_H
#define SCORE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ScoreManager {
public:
    ScoreManager();

    bool fromJSON(const String& json);
    String toJSON() const;
    String formatDisplay() const;

    // Getters
    int  getA()     const { return _a; }
    int  getB()     const { return _b; }
    int  getSetA()  const { return _set_a; }
    int  getSetB()  const { return _set_b; }
    const String& getStatus() const { return _status; }
    const String& getMsg()    const { return _msg; }

private:
    int     _a;
    int     _b;
    int     _set_a;
    int     _set_b;
    String  _status;
    String  _msg;
};

#endif // SCORE_MANAGER_H
