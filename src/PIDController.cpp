#include <Arduino.h>
#include "control/PIDController.h"

PIDController::PIDController(float kp, float ki, float kd)
    : m_kp(kp), m_ki(ki), m_kd(kd) {}

void PIDController::begin() {
    reset();
}

void PIDController::setTunings(float kp, float ki, float kd) {
    m_kp = kp;
    m_ki = ki;
    m_kd = kd;
}

float PIDController::compute(float input) {
    float error = m_setpoint - input;

    m_integral += error * m_Ts;
    m_integral = constrain(m_integral, -m_limit, m_limit);

    float derivative = (error - m_prevError) / m_Ts;
    m_prevError = error;

    float output = m_kp * error + m_ki * m_integral + m_kd * derivative;
    return constrain(output, 0.0f, 1.0f);
}

void PIDController::reset() {
    m_integral = 0.0f;
    m_prevError = 0.0f;
}
