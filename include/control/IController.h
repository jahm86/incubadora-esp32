#pragma once

class IController {
public:
    virtual void begin() = 0;
    virtual void setSetpoint(float setpoint) = 0;
    virtual float compute(float input, float setpoint) = 0;
    virtual void reset() = 0;
    virtual ~IController() = default;
};
