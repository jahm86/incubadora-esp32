#include <Arduino.h>
#include "display/DisplayManager.h"

void DisplayManager::begin() {
    m_tft.init();
    m_tft.setRotation(1);
    m_tft.fillScreen(TFT_BLACK);
    m_tft.setTextFont(2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    resetState();
}

void DisplayManager::clear() {
    m_tft.fillScreen(TFT_BLACK);
    resetState();
}

void DisplayManager::resetState() {
    m_activeScreen = ScreenType::None;
    for (int i = 0; i < 7; i++) {
        m_mainRows[i][0] = '\0';
        m_mainRowColors[i] = 0;
    }
    m_lastMenuTitle = nullptr;
    m_lastMenuSelected = 255;
    m_lastMenuScroll = 255;
    m_lastMenuCount = 255;

    m_lastEditLabel[0] = '\0';
    m_lastEditValue[0] = '\0';

    m_lastInfoTitle[0] = '\0';
    m_lastInfoLine1[0] = '\0';
    m_lastInfoLine2[0] = '\0';

    m_lastConfirmTitle[0] = '\0';
    m_lastConfirmLine[0] = '\0';
    m_lastConfirmChoice = -1;

    m_lastAlarmMessage[0] = '\0';
}

void DisplayManager::switchScreen(ScreenType type) {
    if (m_activeScreen != type) {
        m_tft.fillScreen(TFT_BLACK);
        m_activeScreen = type;
        resetState();
        m_activeScreen = type; // restablecer tras resetState
    }
}

void DisplayManager::drawMainScreen(const SensorData& sensor, const OutputState& outputs,
                                    uint32_t days, uint32_t uptime, float setpoint,
                                    uint32_t turnMinutes, bool turnDisplayUntil) {
    bool isNewScreen = (m_activeScreen != ScreenType::Main);
    switchScreen(ScreenType::Main);

    if (isNewScreen) {
        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString("Incubadora", 60, 4);
        m_tft.drawFastHLine(0, 32, m_tft.width(), TFT_DARKGREY);
    }

    m_tft.setTextFont(2);
    const int step = 26;
    int y = 42;
    char buf[48];
    uint16_t color;
    const uint16_t padWidth = m_tft.width() - 30;

    // Fila 0: Temperatura
    if (sensor.valid) {
        snprintf(buf, sizeof(buf), "Temperatura: %.1f / %.1f C",
                 sensor.temperature, setpoint);
        color = TFT_CYAN;
    } else {
        snprintf(buf, sizeof(buf), "Error en Sensor");
        color = TFT_RED;
    }
    if (strcmp(buf, m_mainRows[0]) != 0 || color != m_mainRowColors[0]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[0], buf, sizeof(m_mainRows[0]) - 1);
        m_mainRowColors[0] = color;
    }
    y += step;

    // Fila 1: Humedad
    if (sensor.valid) {
        snprintf(buf, sizeof(buf), "Humedad:  %.1f %%", sensor.humidity);
        color = TFT_YELLOW;
    } else {
        snprintf(buf, sizeof(buf), "Humedad:  -- %%");
        color = TFT_RED;
    }
    if (strcmp(buf, m_mainRows[1]) != 0 || color != m_mainRowColors[1]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[1], buf, sizeof(m_mainRows[1]) - 1);
        m_mainRowColors[1] = color;
    }
    y += step;

    // Fila 2: Calefactor
    snprintf(buf, sizeof(buf), "Calefactor: %s (%.0f%%)",
             outputs.heaterActive ? "ON" : "OFF",
             outputs.heaterPower * 100.0f);
    color = outputs.heaterActive ? TFT_ORANGE : TFT_DARKGREY;
    if (strcmp(buf, m_mainRows[2]) != 0 || color != m_mainRowColors[2]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[2], buf, sizeof(m_mainRows[2]) - 1);
        m_mainRowColors[2] = color;
    }
    y += step;

    // Fila 3: Humidificador
    snprintf(buf, sizeof(buf), "Humidificador: %s",
             outputs.humidifierActive ? "ON" : "OFF");
    color = outputs.humidifierActive ? TFT_BLUE : TFT_DARKGREY;
    if (strcmp(buf, m_mainRows[3]) != 0 || color != m_mainRowColors[3]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[3], buf, sizeof(m_mainRows[3]) - 1);
        m_mainRowColors[3] = color;
    }
    y += step;

    // Fila 4: Bandeja y Días
    snprintf(buf, sizeof(buf), "Bandeja: %s  Dias: %lu",
             outputs.eggTrayActive ? "ON" : "OFF",
             (unsigned long)days);
    color = TFT_GREEN;
    if (strcmp(buf, m_mainRows[4]) != 0 || color != m_mainRowColors[4]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[4], buf, sizeof(m_mainRows[4]) - 1);
        m_mainRowColors[4] = color;
    }
    y += step;

    // Fila 5: Volteo
    if (turnDisplayUntil) {
        snprintf(buf, sizeof(buf), "Volteo: en %.2lu:%.2lu",
                 (unsigned long)(turnMinutes / 60), (unsigned long)(turnMinutes % 60));
    } else {
        snprintf(buf, sizeof(buf), "Volteo: hace %.2lu:%.2lu",
                 (unsigned long)(turnMinutes / 60), (unsigned long)(turnMinutes % 60));
    }
    color = TFT_MAGENTA;
    if (strcmp(buf, m_mainRows[5]) != 0 || color != m_mainRowColors[5]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[5], buf, sizeof(m_mainRows[5]) - 1);
        m_mainRowColors[5] = color;
    }
    y += step;

    // Fila 6: Tiempo activo
    snprintf(buf, sizeof(buf), "Tiempo activo: %luh %lum",
             (unsigned long)(uptime / 3600),
             (unsigned long)((uptime % 3600) / 60));
    color = TFT_WHITE;
    if (strcmp(buf, m_mainRows[6]) != 0 || color != m_mainRowColors[6]) {
        m_tft.setTextColor(color, TFT_BLACK);
        m_tft.setTextPadding(padWidth);
        m_tft.drawString(buf, 30, y);
        strncpy(m_mainRows[6], buf, sizeof(m_mainRows[6]) - 1);
        m_mainRowColors[6] = color;
    }
}

void DisplayManager::drawMenu(const MenuSystem& menu) {
    bool isNewScreen = (m_activeScreen != ScreenType::Menu);
    switchScreen(ScreenType::Menu);

    const char* title = menu.currentTitle();
    uint8_t count = menu.currentItemCount();
    uint8_t selected = menu.selectedItem();
    uint8_t scroll = menu.scrollOffset();

    bool titleChanged = (title != m_lastMenuTitle);
    if (isNewScreen || titleChanged) {
        if (!isNewScreen) {
            m_tft.fillScreen(TFT_BLACK);
        }
        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString(title, 10, 5, 4);
        m_tft.drawFastHLine(0, 30, m_tft.width(), TFT_WHITE);
        m_lastMenuTitle = title;
        m_lastMenuScroll = 255; // forzar redibujado de filas
    }

    // Si solo cambió la selección sin cambio de scroll ni de título
    if (scroll == m_lastMenuScroll && count == m_lastMenuCount && !isNewScreen && !titleChanged) {
        if (selected != m_lastMenuSelected) {
            // Deseleccionar elemento previo
            if (m_lastMenuSelected >= scroll && m_lastMenuSelected < scroll + MenuSystem::VISIBLE_ROWS) {
                uint8_t oldRow = m_lastMenuSelected - scroll;
                int y = 40 + oldRow * 25;
                m_tft.fillRect(0, y - 2, m_tft.width(), 22, TFT_BLACK);
                m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
                m_tft.setTextPadding(0);
                m_tft.drawString(menu.itemLabel(m_lastMenuSelected), 15, y, 2);
            }
            // Seleccionar nuevo elemento
            if (selected >= scroll && selected < scroll + MenuSystem::VISIBLE_ROWS) {
                uint8_t newRow = selected - scroll;
                int y = 40 + newRow * 25;
                m_tft.fillRect(0, y - 2, m_tft.width(), 22, TFT_BLUE);
                m_tft.setTextColor(TFT_WHITE, TFT_BLUE);
                m_tft.setTextPadding(0);
                m_tft.drawString(menu.itemLabel(selected), 15, y, 2);
            }
            m_lastMenuSelected = selected;
        }
        return;
    }

    // Redibujado de todas las filas visibles
    m_tft.setTextFont(2);
    for (uint8_t i = 0; i < MenuSystem::VISIBLE_ROWS; i++) {
        uint8_t idx = scroll + i;
        int y = 40 + i * 25;
        if (idx < count) {
            bool isSel = (idx == selected);
            m_tft.fillRect(0, y - 2, m_tft.width(), 22, isSel ? TFT_BLUE : TFT_BLACK);
            m_tft.setTextColor(isSel ? TFT_WHITE : TFT_LIGHTGREY, isSel ? TFT_BLUE : TFT_BLACK);
            m_tft.setTextPadding(0);
            m_tft.drawString(menu.itemLabel(idx), 15, y, 2);
        } else {
            m_tft.fillRect(0, y - 2, m_tft.width(), 22, TFT_BLACK);
        }
    }

    if (count > MenuSystem::VISIBLE_ROWS) {
        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString("v scroll", m_tft.width() - 70, 8, 2);
    } else {
        m_tft.fillRect(m_tft.width() - 75, 5, 75, 20, TFT_BLACK);
    }

    m_lastMenuSelected = selected;
    m_lastMenuScroll = scroll;
    m_lastMenuCount = count;
}

void DisplayManager::drawEditValue(const char* label, const char* valueText) {
    bool isNewScreen = (m_activeScreen != ScreenType::EditValue);
    switchScreen(ScreenType::EditValue);

    bool labelChanged = (strcmp(label, m_lastEditLabel) != 0);
    if (isNewScreen || labelChanged) {
        if (!isNewScreen) {
            m_tft.fillScreen(TFT_BLACK);
        }
        m_tft.setTextSize(1);
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString(label, 10, 30, 2);

        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString("Girar: ajustar  Presionar: ok", 10, 180, 2);

        strncpy(m_lastEditLabel, label, sizeof(m_lastEditLabel) - 1);
        m_lastEditValue[0] = '\0';
    }

    if (strcmp(valueText, m_lastEditValue) != 0) {
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
        m_tft.setTextPadding(m_tft.width() - 20);
        m_tft.drawString(valueText, 10, 80, 4);
        strncpy(m_lastEditValue, valueText, sizeof(m_lastEditValue) - 1);
    }
}

void DisplayManager::drawInfo(const char* title, const char* line1, const char* line2) {
    bool isNewScreen = (m_activeScreen != ScreenType::Info);
    switchScreen(ScreenType::Info);

    bool titleChanged = (strcmp(title, m_lastInfoTitle) != 0);
    if (isNewScreen || titleChanged) {
        if (!isNewScreen) {
            m_tft.fillScreen(TFT_BLACK);
        }
        m_tft.setTextSize(1);
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString(title, 10, 30, 2);

        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString("Presionar: volver", 10, 180, 2);

        strncpy(m_lastInfoTitle, title, sizeof(m_lastInfoTitle) - 1);
        m_lastInfoLine1[0] = '\0';
        m_lastInfoLine2[0] = '\0';
    }

    if (strcmp(line1, m_lastInfoLine1) != 0) {
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
        m_tft.setTextPadding(m_tft.width() - 20);
        m_tft.drawString(line1, 10, 80, 4);
        strncpy(m_lastInfoLine1, line1, sizeof(m_lastInfoLine1) - 1);
    }

    if (strcmp(line2, m_lastInfoLine2) != 0) {
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        m_tft.setTextPadding(m_tft.width() - 20);
        m_tft.drawString(line2, 10, 130, 2);
        strncpy(m_lastInfoLine2, line2, sizeof(m_lastInfoLine2) - 1);
    }
}

void DisplayManager::drawConfirm(const char* title, const char* line, bool choice) {
    bool isNewScreen = (m_activeScreen != ScreenType::Confirm);
    switchScreen(ScreenType::Confirm);

    bool textChanged = (strcmp(title, m_lastConfirmTitle) != 0 || strcmp(line, m_lastConfirmLine) != 0);
    if (isNewScreen || textChanged) {
        if (!isNewScreen) {
            m_tft.fillScreen(TFT_BLACK);
        }
        m_tft.setTextSize(1);
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString(title, 10, 30, 2);

        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_RED, TFT_BLACK);
        m_tft.drawString(line, 10, 70, 4);

        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        m_tft.drawString("Girar: cambiar  Presionar: confirmar", 10, 200, 2);

        strncpy(m_lastConfirmTitle, title, sizeof(m_lastConfirmTitle) - 1);
        strncpy(m_lastConfirmLine, line, sizeof(m_lastConfirmLine) - 1);
        m_lastConfirmChoice = -1; // forzar redibujado de botones
    }

    int8_t currentChoice = choice ? 1 : 0;
    if (currentChoice != m_lastConfirmChoice) {
        int y = 170;
        // Botón NO
        m_tft.fillRect(40, y - 2, 60, 22, !choice ? TFT_BLUE : TFT_BLACK);
        m_tft.setTextColor(!choice ? TFT_WHITE : TFT_LIGHTGREY, !choice ? TFT_BLUE : TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString("NO", 45, y, 2);

        // Botón SI
        m_tft.fillRect(140, y - 2, 60, 22, choice ? TFT_BLUE : TFT_BLACK);
        m_tft.setTextColor(choice ? TFT_WHITE : TFT_LIGHTGREY, choice ? TFT_BLUE : TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString("SI", 145, y, 2);

        m_lastConfirmChoice = currentChoice;
    }
}

void DisplayManager::drawAlarm(const char* message) {
    bool isNewScreen = (m_activeScreen != ScreenType::Alarm);
    switchScreen(ScreenType::Alarm);

    if (isNewScreen) {
        m_tft.setTextSize(1);
        m_tft.setTextFont(4);
        m_tft.setTextColor(TFT_RED, TFT_BLACK);
        m_tft.setTextPadding(0);
        m_tft.drawString("ALARMA", 50, 40, 4);

        m_tft.setTextFont(2);
        m_tft.drawString("Presione boton", 30, 140, 2);
        m_lastAlarmMessage[0] = '\0';
    }

    if (strcmp(message, m_lastAlarmMessage) != 0) {
        m_tft.setTextFont(2);
        m_tft.setTextColor(TFT_RED, TFT_BLACK);
        m_tft.setTextPadding(m_tft.width() - 20);
        m_tft.drawString(message, 10, 100, 2);
        strncpy(m_lastAlarmMessage, message, sizeof(m_lastAlarmMessage) - 1);
    }
}
