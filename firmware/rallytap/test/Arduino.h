/**
 * Minimal Arduino.h stub for native (host) compilation.
 *
 * Provides just enough of the Arduino String type and platform macros
 * so that ScoreManager and related headers compile on the host machine.
 * Not a full emulation — only what the tested code actually uses.
 *
 * ArduinoJson v7 requires String to have read() (for deserialization)
 * and write() (for serialization) — these are implemented here.
 */
#ifndef STUB_ARDUINO_H
#define STUB_ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// Basic Arduino type aliases
// ---------------------------------------------------------------------------

typedef uint8_t  byte;
typedef uint16_t word;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

enum {
    HIGH         = 0x1,
    LOW          = 0x0,
    INPUT        = 0x0,
    OUTPUT       = 0x1,
    INPUT_PULLUP = 0x2,
};

// ---------------------------------------------------------------------------
// Time stubs
// ---------------------------------------------------------------------------

static inline unsigned long millis() { return 0; }
static inline void delay(unsigned long) {}

// ---------------------------------------------------------------------------
// GPIO stubs
// ---------------------------------------------------------------------------

static inline void pinMode(uint8_t, uint8_t) {}
static inline int  digitalRead(uint8_t) { return HIGH; }
static inline void digitalWrite(uint8_t, uint8_t) {}

// ---------------------------------------------------------------------------
// Serial stub
// ---------------------------------------------------------------------------

class SerialClass {
public:
    void begin(unsigned long) {}
    void end() {}
    int  available() { return 0; }
    int  read() { return -1; }
    size_t print(const char*) { return 0; }
    size_t print(int) { return 0; }
    size_t print(unsigned int) { return 0; }
    size_t print(long) { return 0; }
    size_t print(unsigned long) { return 0; }
    size_t print(double) { return 0; }
    size_t print(const class String&) { return 0; }
    size_t println() { return 0; }
    size_t println(const char*) { return 0; }
    size_t println(int) { return 0; }
    size_t println(unsigned int) { return 0; }
    size_t println(long) { return 0; }
    size_t println(unsigned long) { return 0; }
    size_t println(double) { return 0; }
    size_t println(const class String&) { return 0; }
};
static SerialClass Serial;

// ---------------------------------------------------------------------------
// Minimal String class — provides enough for ScoreManager + ArduinoJson v7
// ---------------------------------------------------------------------------

class String {
public:
    // -- constructors --
    String() : _buf(nullptr), _len(0), _readPos(0) {}

    String(const char* cstr) : _readPos(0) {
        if (cstr) {
            _len = strlen(cstr);
            _buf = new char[_len + 1];
            memcpy(_buf, cstr, _len + 1);
        } else {
            _buf = nullptr;
            _len = 0;
        }
    }

    String(int val) : _readPos(0) {
        // Max int fits in 12 chars
        _buf = new char[16];
        _len = snprintf(_buf, 16, "%d", val);
    }

    String(unsigned int val) : _readPos(0) {
        _buf = new char[16];
        _len = snprintf(_buf, 16, "%u", val);
    }

    String(long val) : _readPos(0) {
        _buf = new char[16];
        _len = snprintf(_buf, 16, "%ld", val);
    }

    String(unsigned long val) : _readPos(0) {
        _buf = new char[16];
        _len = snprintf(_buf, 16, "%lu", val);
    }

    // -- copy --
    String(const String& other) : _buf(nullptr), _len(0), _readPos(0) {
        *this = other;
    }

    // -- assignment --
    String& operator=(const String& rhs) {
        if (this != &rhs) {
            delete[] _buf;
            _len = rhs._len;
            if (rhs._buf) {
                _buf = new char[_len + 1];
                memcpy(_buf, rhs._buf, _len + 1);
            } else {
                _buf = nullptr;
            }
            _readPos = 0;   // reset read position on assignment
        }
        return *this;
    }

    String& operator=(const char* cstr) {
        delete[] _buf;
        if (cstr) {
            _len = strlen(cstr);
            _buf = new char[_len + 1];
            memcpy(_buf, cstr, _len + 1);
        } else {
            _buf = nullptr;
            _len = 0;
        }
        _readPos = 0;
        return *this;
    }

    // -- destructor --
    ~String() { delete[] _buf; }

    // -- access --
    const char* c_str() const { return _buf ? _buf : ""; }
    unsigned int length() const { return _len; }

    // -- ArduinoJson v7 support: sequential character reading --
    // ArduinoJson's Reader<const String> calls source->read() to consume input
    int read() const {
        if (!_buf || _readPos >= _len) return -1;
        return static_cast<unsigned char>(_buf[_readPos++]);
    }

    // -- ArduinoJson v7 support: sequential character writing --
    // ArduinoJson's Writer<String> calls dest->write(c) and dest->write(s, n)
    size_t write(uint8_t c) {
        char* newBuf = new char[_len + 2];
        if (_buf) {
            memcpy(newBuf, _buf, _len);
            delete[] _buf;
        }
        newBuf[_len] = static_cast<char>(c);
        newBuf[_len + 1] = '\0';
        _buf = newBuf;
        _len++;
        return 1;
    }

    size_t write(const uint8_t* s, size_t n) {
        if (n == 0) return 0;
        char* newBuf = new char[_len + n + 1];
        if (_buf) {
            memcpy(newBuf, _buf, _len);
            delete[] _buf;
        }
        memcpy(newBuf + _len, s, n);
        newBuf[_len + n] = '\0';
        _buf = newBuf;
        _len += n;
        return n;
    }

    // -- concatenation --
    String operator+(const String& rhs) const {
        String result;
        result._len = _len + rhs._len;
        result._buf = new char[result._len + 1];
        if (_buf) memcpy(result._buf, _buf, _len);
        if (rhs._buf) memcpy(result._buf + _len, rhs._buf, rhs._len);
        result._buf[result._len] = '\0';
        return result;
    }

    String operator+(const char* rhs) const {
        return *this + String(rhs);
    }

    String operator+(int rhs) const {
        return *this + String(rhs);
    }

    // -- comparison --
    bool operator==(const char* cstr) const {
        if (!_buf && !cstr) return true;
        if (!_buf || !cstr) return false;
        return strcmp(_buf, cstr) == 0;
    }

    bool operator==(const String& rhs) const {
        if (!_buf && !rhs._buf) return true;
        if (!_buf || !rhs._buf) return false;
        return strcmp(_buf, rhs._buf) == 0;
    }

    // -- substring --
    String substring(unsigned int begin, unsigned int end) const {
        if (!_buf || begin >= _len || begin >= end) return String();
        unsigned int count = std::min(end, _len) - begin;
        char* tmp = new char[count + 1];
        memcpy(tmp, _buf + begin, count);
        tmp[count] = '\0';
        String result(tmp);
        delete[] tmp;
        return result;
    }

    // -- indexing --
    char operator[](unsigned int idx) const {
        return (_buf && idx < _len) ? _buf[idx] : '\0';
    }

private:
    char*          _buf;
    unsigned int   _len;
    mutable size_t _readPos;   // read cursor for ArduinoJson v7
};

// -- non-member concatenation helpers --
static inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

static inline bool operator==(const char* lhs, const String& rhs) {
    return rhs == lhs;
}

static inline bool operator!=(const String& lhs, const char* rhs) {
    return !(lhs == rhs);
}

static inline bool operator!=(const char* lhs, const String& rhs) {
    return !(rhs == lhs);
}

// ---------------------------------------------------------------------------
// random() — deterministic LCG so host tests can exercise backoff jitter.
// Same [min, max) contract as the Arduino core.
// ---------------------------------------------------------------------------

static unsigned long _randState = 1;

static inline unsigned long random(unsigned long howbig) {
    if (howbig == 0) return 0;
    _randState = _randState * 1103515245UL + 12345UL;
    return (_randState >> 16) % howbig;
}

static inline long random(long howsmall, long howbig) {
    if (howbig <= howsmall) return howsmall;
    return howsmall + random(static_cast<unsigned long>(howbig - howsmall));
}

// ---------------------------------------------------------------------------
// No abs() macro is provided on purpose: the Arduino core defines abs as a
// function-like macro, but on host builds that macro corrupts libstdc++'s
// internal headers (g++ <tr1/bessel_function.tcc> etc.) where abs appears as
// a function name, producing "expected unqualified-id before '(' token".
// Nothing in the tested code calls abs(), and std::abs is available via
// <stdlib.h> above, so the macro is simply omitted.
// ---------------------------------------------------------------------------

#endif // STUB_ARDUINO_H
