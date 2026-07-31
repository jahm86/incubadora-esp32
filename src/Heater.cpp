#include <Arduino.h>
#include "output/Heater.h"

void Heater::begin() {
    pinMode(PIN_HEATER, OUTPUT);
    off();
}

void Heater::setPower(float power) {
    m_power = constrain(power, 0.0f, 1.0f);
    uint32_t duty = static_cast<uint32_t>(m_power * 255.0f);
    analogWrite(PIN_HEATER, duty);
}
