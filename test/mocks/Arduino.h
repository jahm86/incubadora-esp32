#pragma once

#ifndef ARDUINO
#define ARDUINO 10808
#endif

#ifdef __cplusplus
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>

class String : public std::string {
public:
    using std::string::string;
    String() : std::string() {}
    String(const std::string& s) : std::string(s) {}
    String(const char* s) : std::string(s ? s : "") {}
    String(int v) : std::string(std::to_string(v)) {}
    String(unsigned int v) : std::string(std::to_string(v)) {}
    String(long v) : std::string(std::to_string(v)) {}
    String(unsigned long v) : std::string(std::to_string(v)) {}
    String(float v) : std::string(std::to_string(v)) {}

    const char* c_str() const { return std::string::c_str(); }
    size_t length() const { return std::string::length(); }
    void trim() {
        while (!empty() && isspace(static_cast<unsigned char>(front()))) erase(begin());
        while (!empty() && isspace(static_cast<unsigned char>(back()))) pop_back();
    }
    String substring(size_t from, size_t to = std::string::npos) const {
        if (from >= length()) return String("");
        if (to == std::string::npos || to > length()) return String(substr(from));
        return String(substr(from, to - from));
    }
    bool startsWith(const String& prefix) const {
        if (prefix.length() > length()) return false;
        return compare(0, prefix.length(), prefix) == 0;
    }
    float toFloat() const {
        try { return std::stof(*this); } catch (...) { return 0.0f; }
    }
    int toInt() const {
        try { return std::stoi(*this); } catch (...) { return 0; }
    }
};

template <typename T>
static inline T constrain(T amt, T low, T high) {
    return (amt < low) ? low : ((amt > high) ? high : amt);
}

// Simulador de tiempo en millis
inline uint32_t& mock_millis_ref() {
    static uint32_t s_millis = 0;
    return s_millis;
}

inline uint32_t millis() {
    return mock_millis_ref();
}

inline void mock_set_millis(uint32_t ms) {
    mock_millis_ref() = ms;
}

inline void mock_advance_millis(uint32_t ms) {
    mock_millis_ref() += ms;
}

inline void delay(uint32_t ms) {
    mock_advance_millis(ms);
}

// GPIO Pin Mocking
#define OUTPUT 1
#define INPUT 0
#define LOW 0
#define HIGH 1

inline uint8_t& mock_pin_mode_ref(uint8_t pin) {
    static uint8_t s_pinMode[64] = {0};
    return s_pinMode[pin < 64 ? pin : 0];
}

inline uint8_t& mock_digital_pin_ref(uint8_t pin) {
    static uint8_t s_pinVal[64] = {0};
    return s_pinVal[pin < 64 ? pin : 0];
}

inline void pinMode(uint8_t pin, uint8_t mode) {
    mock_pin_mode_ref(pin) = mode;
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
    mock_digital_pin_ref(pin) = val;
}

inline uint8_t digitalRead(uint8_t pin) {
    return mock_digital_pin_ref(pin);
}

inline void mock_reset_pins() {
    for (int i = 0; i < 64; ++i) {
        mock_pin_mode_ref(i) = 0;
        mock_digital_pin_ref(i) = 0;
    }
}

#endif // __cplusplus
