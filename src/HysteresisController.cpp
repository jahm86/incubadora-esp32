#include <Arduino.h>
#include "control/HysteresisController.h"

HysteresisController::HysteresisController(float hysteresis)
    : m_hysteresis(hysteresis) {}

void HysteresisController::begin() {
    m_heaterOn = false;
}

void HysteresisController::setHysteresis(float hysteresis) {
    m_hysteresis = hysteresis;
}

float HysteresisController::compute(float input) {
    float halfBand = m_hysteresis / 2.0f;
    float upper = m_setpoint + halfBand;
    float lower = m_setpoint - halfBand;

    if (m_heaterOn && input >= upper) {
        m_heaterOn = false;
    } else if (!m_heaterOn && input <= lower) {
        m_heaterOn = true;
    }

    return m_heaterOn ? 1.0f : 0.0f;
}

void HysteresisController::reset() {
    m_heaterOn = false;
}
