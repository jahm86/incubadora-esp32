#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "types.h"
#include "config/settings.h"
#include "display/MenuSystem.h"

class DisplayManager {
public:
    void begin();
    void clear();

    void drawMainScreen(const SensorData& sensor, const OutputState& outputs,
                        uint32_t days, uint32_t uptime, float setpoint);
    void drawMenu(const MenuSystem& menu);
    void drawEditValue(const char* label, const char* valueText);
    void drawAlarm(const char* message);

    TFT_eSPI& tft() { return m_tft; }

private:
    TFT_eSPI m_tft;
};
