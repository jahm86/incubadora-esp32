#pragma once

#include "IController.h"

class LADRCController : public IController {
public:
    LADRCController(float b0, float wc, float wo)
        : m_b0(b0), m_wc(wc), m_wo(wo) {}

    void begin() override {
        reset();
    }

    void setSetpoint(float setpoint) override {
        m_setpoint = setpoint;
    }

    void setTunings(float b0, float wc, float wo) {
        m_b0 = b0;
        m_wc = wc;
        m_wo = wo;
    }

    float compute(float input, float setpoint) override {
        float e = input - m_z1;
        m_z1 += m_Ts * (m_z2 + m_b0 * m_lastU + 2.0f * m_wo * e);
        m_z2 += m_Ts * (m_wo * m_wo * e);

        float u = (m_wc * (setpoint - m_z1) - m_z2) / m_b0;
        m_lastU = constrain(u, 0.0f, 1.0f);
        return m_lastU;
    }

    void reset() override {
        m_z1 = 0.0f;
        m_z2 = 0.0f;
        m_lastU = 0.0f;
    }

private:
    float m_b0, m_wc, m_wo;
    float m_setpoint = 0.0f;
    float m_z1 = 0.0f;
    float m_z2 = 0.0f;
    float m_lastU = 0.0f;
};
