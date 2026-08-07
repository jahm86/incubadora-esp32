#include <Arduino.h>
#include "display/DisplayManager.h"

void DisplayManager::begin() {
    m_tft.init();
    m_tft.setRotation(1);
    m_tft.fillScreen(TFT_BLACK);
    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void DisplayManager::clear() {
    m_tft.fillScreen(TFT_BLACK);
}

void DisplayManager::drawMainScreen(const SensorData& sensor, const OutputState& outputs,
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

    if (sensor.valid) {
        snprintf(buf, sizeof(buf), "Hum:  %.1f %%", sensor.humidity);
        m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    } else {
        snprintf(buf, sizeof(buf), "Hum:  -- %%");
        m_tft.setTextColor(TFT_RED, TFT_BLACK);
    }
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

void DisplayManager::drawMenu(const MenuSystem& menu) {
    clear();

    m_tft.setTextSize(1);
    m_tft.setTextFont(4);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(menu.currentTitle(), 10, 5, 4);

    m_tft.drawFastHLine(0, 30, 240, TFT_WHITE);

    m_tft.setTextFont(2);
    uint8_t count = menu.currentItemCount();
    uint8_t selected = menu.selectedItem();
    uint8_t scroll = menu.scrollOffset();

    for (uint8_t i = 0; i < MenuSystem::VISIBLE_ROWS; i++) {
        uint8_t idx = scroll + i;
        if (idx >= count) {
            break;
        }
        int y = 40 + i * 25;
        if (idx == selected) {
            m_tft.fillRect(0, y - 2, 240, 20, TFT_BLUE);
            m_tft.setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        }
        m_tft.drawString(menu.itemLabel(idx), 15, y, 2);
    }

    if (count > MenuSystem::VISIBLE_ROWS) {
        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString("v scroll", 180, 265, 2);
    }
}

void DisplayManager::drawEditValue(const char* label, const char* valueText) {
    clear();

    m_tft.setTextSize(1);
    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(label, 10, 30, 2);

    m_tft.setTextFont(4);
    m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
    m_tft.drawString(valueText, 10, 80, 4);

    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    m_tft.drawString("Girar: ajustar  Presionar: ok", 10, 180, 2);
}

void DisplayManager::drawInfo(const char* title, const char* line1, const char* line2) {
    clear();

    m_tft.setTextSize(1);
    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(title, 10, 30, 2);

    m_tft.setTextFont(4);
    m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
    m_tft.drawString(line1, 10, 80, 4);

    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    m_tft.drawString(line2, 10, 130, 2);

    m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    m_tft.drawString("Presionar: volver", 10, 180, 2);
}

void DisplayManager::drawConfirm(const char* title, const char* line, bool choice) {
    clear();

    m_tft.setTextSize(1);
    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(title, 10, 30, 2);

    m_tft.setTextFont(4);
    m_tft.setTextColor(TFT_RED, TFT_BLACK);
    m_tft.drawString(line, 10, 70, 4);

    int y = 170;
    if (!choice) {
        m_tft.fillRect(40, y - 2, 60, 20, TFT_BLUE);
        m_tft.setTextColor(TFT_WHITE, TFT_BLUE);
    } else {
        m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    }
    m_tft.drawString("NO", 45, y, 2);

    if (choice) {
        m_tft.fillRect(140, y - 2, 60, 20, TFT_BLUE);
        m_tft.setTextColor(TFT_WHITE, TFT_BLUE);
    } else {
        m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    }
    m_tft.drawString("SI", 145, y, 2);

    m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    m_tft.drawString("Girar: cambiar  Presionar: confirmar", 10, 200, 2);
}

void DisplayManager::drawAlarm(const char* message) {
    clear();

    m_tft.setTextSize(1);
    m_tft.setTextFont(4);
    m_tft.setTextColor(TFT_RED, TFT_BLACK);
    m_tft.drawString("ALARMA", 50, 40, 4);

    m_tft.setTextFont(2);
    m_tft.drawString(message, 10, 100, 2);
    m_tft.drawString("Presione boton", 30, 140, 2);
}
