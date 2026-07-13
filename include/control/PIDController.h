#pragma once

#include "IController.h"

class PIDController : public IController {
public:
    PIDController(float kp, float ki, float kd)
        : m_kp(kp), m_ki(ki), m_kd(kd) {}

    void begin() override {
        reset();
    }

    void setSetpoint(float setpoint) override {
        m_setpoint = setpoint;
    }

    void setTunings(float kp, float ki, float kd) {
        m_kp = kp;
        m_ki = ki;
        m_kd = kd;
    }

    float compute(float input, float setpoint) override {
        float error = setpoint - input;

        m_integral += error * m_dt;
        m_integral = constrain(m_integral, -m_limit, m_limit);

        float derivative = (error - m_prevError) / m_dt;
        m_prevError = error;

        float output = m_kp * error + m_ki * m_integral + m_kd * derivative;
        return constrain(output, 0.0f, 1.0f);
    }

    void reset() override {
        m_integral = 0.0f;
        m_prevError = 0.0f;
    }

private:
    float m_kp, m_ki, m_kd;
    float m_setpoint = 0.0f;
    float m_integral = 0.0f;
    float m_prevError = 0.0f;
    float m_limit = 100.0f;
    float m_dt = 1.0f;
};
