#pragma once

#include <Arduino.h>
#include "config/pins.h"

class EggTray {
public:
    void begin();
    void on();
    void off();
    bool isActive() const { return m_active; }

private:
    bool m_active = false;
};
