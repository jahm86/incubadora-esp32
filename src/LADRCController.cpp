#include <Arduino.h>
#include "control/LADRCController.h"

LADRCController::LADRCController(float b0, float wc, float wo)
    : m_b0(b0), m_wc(wc), m_wo(wo) {}

void LADRCController::begin() {
    reset();
}

void LADRCController::setTunings(float b0, float wc, float wo) {
    m_b0 = b0;
    m_wc = wc;
    m_wo = wo;
}

float LADRCController::compute(float input) {
    float e = input - m_z1;
    m_z1 += m_Ts * (m_z2 + m_b0 * m_lastU + 2.0f * m_wo * e);
    m_z2 += m_Ts * (m_wo * m_wo * e);

    float u = (m_wc * (m_setpoint - m_z1) - m_z2) / m_b0;
    m_lastU = constrain(u, 0.0f, 1.0f);
    return m_lastU;
}

void LADRCController::reset() {
    m_z1 = 0.0f;
    m_z2 = 0.0f;
    m_lastU = 0.0f;
}
