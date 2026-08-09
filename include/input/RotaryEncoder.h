#pragma once

#include <Arduino.h>
#include <ESP32RotaryEncoder.h>
#include "config/pins.h"

class RotaryEncoderInput {
public:
    RotaryEncoderInput();

    void begin();
    void setBoundaries(long minVal, long maxVal, bool wrap);
    void setValue(long val);
    long getValue();
    bool isPressed();
    int pollButton();
    void loop();

private:
    RotaryEncoder m_encoder;
    bool m_lastButtonState = false;
};
