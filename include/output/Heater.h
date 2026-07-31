#pragma once

#include <Arduino.h>
#include "config/pins.h"

class Heater {
public:
    void begin();
    void setPower(float power);
    float getPower() const { return m_power; }
    void on() { setPower(1.0f); }
    void off() { setPower(0.0f); }
    bool isActive() const { return m_power > 0.01f; }

private:
    float m_power = 0.0f;
};
