#pragma once

class IController {
public:
    virtual void begin() = 0;
    virtual void setSetpoint(float setpoint) { m_setpoint = setpoint; }
    virtual void setTs(float seconds) { m_Ts = seconds; }
    virtual float compute(float input) = 0;
    virtual void reset() = 0;
    virtual ~IController() = default;

protected:
    float m_Ts = 1.0f;
    float m_setpoint = 0.0f;
};
