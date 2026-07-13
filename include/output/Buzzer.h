#pragma once

#include <Arduino.h>
#include "config/pins.h"

class Buzzer {
public:
    void begin() {
        pinMode(PIN_BUZZER, OUTPUT);
        off();
    }

    void on() {
        digitalWrite(PIN_BUZZER, HIGH);
    }

    void off() {
        digitalWrite(PIN_BUZZER, LOW);
    }

    void beep(uint32_t durationMs) {
        on();
        delay(durationMs);
        off();
    }

    void alarmPattern() {
        for (int i = 0; i < 3; i++) {
            on();
            delay(200);
            off();
            delay(100);
        }
    }

private:
};
