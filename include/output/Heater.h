#pragma once

#include <Arduino.h>
#include "config/pins.h"

class Heater {
public:
    void begin() {
        pinMode(PIN_HEATER, OUTPUT);
        off();
    }

    void setPower(float power) {
        m_power = constrain(power, 0.0f, 1.0f);
        uint32_t duty = static_cast<uint32_t>(m_power * 255.0f);
        analogWrite(PIN_HEATER, duty);
    }

    float getPower() const { return m_power; }

    void on() { setPower(1.0f); }
    void off() { setPower(0.0f); }

    bool isActive() const { return m_power > 0.01f; }

private:
    float m_power = 0.0f;
};
