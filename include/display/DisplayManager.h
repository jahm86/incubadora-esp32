#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "types.h"
#include "config/settings.h"

class DisplayManager {
public:
    DisplayManager() : m_tft() {}

    void begin() {
        m_tft.init();
        m_tft.setRotation(1);
        m_tft.fillScreen(TFT_BLACK);
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    void clear() {
        m_tft.fillScreen(TFT_BLACK);
    }

    void drawMainScreen(const SensorData& sensor, const OutputState& outputs,
                        uint32_t days, uint32_t uptime, float setpoint) {
        clear();

        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);

        int y = 10;
        m_tft.drawString("Incubadora", 10, y, 4);
        y += 30;

        m_tft.setTextFont(2);
        char buf[32];

        if (sensor.valid) {
            snprintf(buf, sizeof(buf), "Temp: %.1f / %.1f C",
                     sensor.temperature, setpoint);
            m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
            m_tft.drawString(buf, 10, y, 2);
        } else {
            m_tft.setTextColor(TFT_RED, TFT_BLACK);
            m_tft.drawString("Sensor Error", 10, y, 2);
        }
        y += 20;

        snprintf(buf, sizeof(buf), "Hum:  %.1f %%", sensor.humidity);
        m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        m_tft.drawString(buf, 10, y, 2);
        y += 20;

        snprintf(buf, sizeof(buf), "Heater: %s (%.0f%%)",
                 outputs.heaterActive ? "ON" : "OFF",
                 outputs.heaterPower * 100.0f);
        m_tft.setTextColor(outputs.heaterActive ? TFT_ORANGE : TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString(buf, 10, y, 2);
        y += 20;

        snprintf(buf, sizeof(buf), "Humidifier: %s",
                 outputs.humidifierActive ? "ON" : "OFF");
        m_tft.setTextColor(outputs.humidifierActive ? TFT_BLUE : TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString(buf, 10, y, 2);
        y += 20;

        snprintf(buf, sizeof(buf), "Tray: %s  Days: %lu",
                 outputs.eggTrayActive ? "ON" : "OFF",
                 (unsigned long)days);
        m_tft.setTextColor(TFT_GREEN, TFT_BLACK);
        m_tft.drawString(buf, 10, y, 2);
        y += 20;

        snprintf(buf, sizeof(buf), "Uptime: %luh %lum",
                 (unsigned long)(uptime / 3600),
                 (unsigned long)((uptime % 3600) / 60));
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.drawString(buf, 10, y, 2);
    }

    void drawMenu(const char* title, const char* const* items, uint8_t count,
                  uint8_t selected) {
        clear();
        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.drawString(title, 10, 5, 4);

        m_tft.drawFastHLine(0, 30, 240, TFT_WHITE);

        m_tft.setTextFont(2);
        for (uint8_t i = 0; i < count; i++) {
            int y = 40 + i * 25;
            if (i == selected) {
                m_tft.fillRect(0, y - 2, 240, 20, TFT_BLUE);
                m_tft.setTextColor(TFT_WHITE, TFT_BLUE);
            } else {
                m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            m_tft.drawString(items[i], 15, y, 2);
        }
    }

    void drawEditValue(const char* label, float value, const char* unit) {
        clear();
        m_tft.setTextSize(1);
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.drawString(label, 10, 30, 2);

        m_tft.setTextFont(6);
        m_tft.setTextColor(TFT_CYAN, TFT_BLACK);

        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f %s", value, unit);
        m_tft.drawString(buf, 10, 70, 6);

        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString("Girar: ajustar   Presionar: ok", 10, 180, 2);
    }

    void drawAlarm(const char* message) {
        clear();
        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_RED, TFT_BLACK);
        m_tft.drawString("ALARMA", 50, 40, 4);

        m_tft.setTextFont(2);
        m_tft.drawString(message, 10, 100, 2);
        m_tft.drawString("Presione boton", 30, 140, 2);
    }

    TFT_eSPI& tft() { return m_tft; }

private:
    TFT_eSPI m_tft;
};
