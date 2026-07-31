#pragma once

#include "IController.h"

class LADRCController : public IController {
public:
    LADRCController(float b0, float wc, float wo);

    void begin() override;
    void setTunings(float b0, float wc, float wo);
    float compute(float input) override;
    void reset() override;

private:
    float m_b0, m_wc, m_wo;
    float m_z1 = 0.0f;
    float m_z2 = 0.0f;
    float m_lastU = 0.0f;
};
