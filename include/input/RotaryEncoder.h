#pragma once

#include <Arduino.h>
#include <ESP32RotaryEncoder.h>
#include "config/pins.h"

class RotaryEncoderInput {
public:
    RotaryEncoderInput()
        : m_encoder(PIN_ENC_CLK, PIN_ENC_DT, PIN_ENC_SW) {}

    void begin() {
        m_encoder.begin();
        m_encoder.setBoundaries(0, 255, true);
        m_encoder.setEncoderValue(0);
    }

    void setBoundaries(long minVal, long maxVal, bool wrap) {
        m_encoder.setBoundaries(minVal, maxVal, wrap);
    }

    void setValue(long val) {
        m_encoder.setEncoderValue(val);
    }

    long getValue() {
        return m_encoder.getEncoderValue();
    }

    bool isPressed() {
        return m_encoder.buttonPressed();
    }

    bool wasClicked() {
        bool pressed = m_encoder.buttonPressed();
        if (pressed && !m_lastButtonState) {
            m_lastButtonState = true;
            return false;
        }
        if (!pressed && m_lastButtonState) {
            m_lastButtonState = false;
            return true;
        }
        return false;
    }

    void loop() {
        m_encoder.loop();
    }

private:
    RotaryEncoder m_encoder;
    bool m_lastButtonState = false;
};
