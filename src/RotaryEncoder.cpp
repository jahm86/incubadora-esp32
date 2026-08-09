#include <Arduino.h>
#include "input/RotaryEncoder.h"

RotaryEncoderInput::RotaryEncoderInput()
    : m_encoder(PIN_ENC_CLK, PIN_ENC_DT, PIN_ENC_SW) {}

void RotaryEncoderInput::begin() {
    m_encoder.begin();
    m_encoder.setBoundaries(0, 255, true);
    m_encoder.setEncoderValue(0);
}

void RotaryEncoderInput::setBoundaries(long minVal, long maxVal, bool wrap) {
    m_encoder.setBoundaries(minVal, maxVal, wrap);
}

void RotaryEncoderInput::setValue(long val) {
    m_encoder.setEncoderValue(val);
}

long RotaryEncoderInput::getValue() {
    return m_encoder.getEncoderValue();
}

bool RotaryEncoderInput::isPressed() {
    return m_encoder.buttonPressed();
}

int RotaryEncoderInput::pollButton() {
    bool pressed = m_encoder.buttonPressed();
    if (pressed != m_lastButtonState) {
        m_lastButtonState = pressed;
        return pressed ? 1 : -1;
    }
    return 0;
}

void RotaryEncoderInput::loop() {
    m_encoder.loop();
}
