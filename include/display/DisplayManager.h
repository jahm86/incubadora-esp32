#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "types.h"
#include "config/settings.h"
#include "display/MenuSystem.h"

enum class ScreenType : uint8_t {
    None,
    Main,
    Menu,
    EditValue,
    Info,
    Confirm,
    Alarm
};

class DisplayManager {
public:
    void begin();
    void clear();
    void resetState();

    void drawMainScreen(const SensorData& sensor, const OutputState& outputs,
                        uint32_t days, uint32_t uptime, float setpoint,
                        uint32_t turnMinutes, bool turnDisplayUntil);
    void drawMenu(const MenuSystem& menu);
    void drawEditValue(const char* label, const char* valueText);
    void drawInfo(const char* title, const char* line1, const char* line2);
    void drawConfirm(const char* title, const char* line, bool choice);
    void drawAlarm(const char* message);

    TFT_eSPI& tft() { return m_tft; }

private:
    TFT_eSPI m_tft;
    ScreenType m_activeScreen = ScreenType::None;

    // Cache para MainScreen (7 filas)
    char     m_mainRows[7][48] = {{0}};
    uint16_t m_mainRowColors[7] = {0};

    // Cache para Menu
    const char* m_lastMenuTitle    = nullptr;
    uint8_t     m_lastMenuSelected = 255;
    uint8_t     m_lastMenuScroll   = 255;
    uint8_t     m_lastMenuCount    = 255;

    // Cache para EditValue
    char m_lastEditLabel[32] = {0};
    char m_lastEditValue[24] = {0};

    // Cache para Info
    char m_lastInfoTitle[24] = {0};
    char m_lastInfoLine1[24] = {0};
    char m_lastInfoLine2[24] = {0};

    // Cache para Confirm
    char   m_lastConfirmTitle[24] = {0};
    char   m_lastConfirmLine[32]  = {0};
    int8_t m_lastConfirmChoice    = -1;

    // Cache para Alarm
    char m_lastAlarmMessage[64] = {0};

    void switchScreen(ScreenType type);
};
