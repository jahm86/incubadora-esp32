#pragma once

#include "IController.h"

class PIDController : public IController {
public:
    PIDController(float kp, float ki, float kd);

    void begin() override;
    void setTunings(float kp, float ki, float kd);
    float compute(float input) override;
    void reset() override;

private:
    float m_kp, m_ki, m_kd;
    float m_integral = 0.0f;
    float m_prevError = 0.0f;
    float m_limit = 100.0f;
};
