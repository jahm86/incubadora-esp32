#pragma once

#include "IController.h"

class HysteresisController : public IController {
public:
    HysteresisController(float hysteresis);

    void begin() override;
    void setHysteresis(float hysteresis);
    float compute(float input) override;
    void reset() override;

    bool isHeaterOn() const { return m_heaterOn; }

private:
    float m_hysteresis;
    bool  m_heaterOn = false;
};
