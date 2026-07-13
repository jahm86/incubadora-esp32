#pragma once

#include "IController.h"

class ARDCController : public IController {
public:
    ARDCController(float hysteresis) : m_hysteresis(hysteresis) {}

    void begin() override {
        m_heaterOn = false;
    }

    void setSetpoint(float setpoint) override {
        m_setpoint = setpoint;
    }

    void setHysteresis(float hysteresis) {
        m_hysteresis = hysteresis;
    }

    float compute(float input, float setpoint) override {
        float halfBand = m_hysteresis / 2.0f;
        float upper = setpoint + halfBand;
        float lower = setpoint - halfBand;

        if (m_heaterOn && input >= upper) {
            m_heaterOn = false;
        } else if (!m_heaterOn && input <= lower) {
            m_heaterOn = true;
        }

        return m_heaterOn ? 1.0f : 0.0f;
    }

    void reset() override {
        m_heaterOn = false;
    }

    bool isHeaterOn() const { return m_heaterOn; }

private:
    float m_hysteresis;
    float m_setpoint = 0.0f;
    bool  m_heaterOn = false;
};
