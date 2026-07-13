#pragma once

#include <Arduino.h>
#include "config/pins.h"

class EggTray {
public:
    void begin() {
        pinMode(PIN_EGG_TRAY, OUTPUT);
        off();
    }

    void on() {
        digitalWrite(PIN_EGG_TRAY, HIGH);
        m_active = true;
    }

    void off() {
        digitalWrite(PIN_EGG_TRAY, LOW);
        m_active = false;
    }

    bool isActive() const { return m_active; }

private:
    bool m_active = false;
};
