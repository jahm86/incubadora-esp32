#pragma once

#include <Arduino.h>
#include "config/pins.h"

class Humidifier {
public:
    void begin() {
        pinMode(PIN_HUMIDIFIER, OUTPUT);
        off();
    }

    void on() {
        digitalWrite(PIN_HUMIDIFIER, HIGH);
        m_active = true;
    }

    void off() {
        digitalWrite(PIN_HUMIDIFIER, LOW);
        m_active = false;
    }

    bool isActive() const { return m_active; }

private:
    bool m_active = false;
};
