#pragma once

#include <Arduino.h>
#include <functional>
#include "config/settings.h"
#include "types.h"

class MenuSystem {
public:
    using ActionCallback = std::function<void()>;

    enum class Action {
        None,
        ValueChanged,
        EnterSubmenu,
        Exit
    };

    MenuSystem() = default;

    void begin() {
        m_currentPage = MenuPage::Main;
        m_selectedItem = 0;
        m_editMode = false;
    }

    MenuPage currentPage() const { return m_currentPage; }
    uint8_t selectedItem() const { return m_selectedItem; }
    bool isEditing() const { return m_editMode; }

    void navigate(int delta) {
        if (m_currentPage == MenuPage::EditValue) {
            if (m_onValueChange) {
                m_onValueChange(delta);
            }
            return;
        }

        int count = getCurrentItemCount();
        int newSel = m_selectedItem + delta;
        if (newSel < 0) newSel = count - 1;
        if (newSel >= count) newSel = 0;
        m_selectedItem = newSel;
    }

    Action confirm() {
        if (m_currentPage == MenuPage::Main) {
            if (m_selectedItem == 0) {
                m_currentPage = MenuPage::Config;
                m_selectedItem = 0;
                return Action::EnterSubmenu;
            }
            return Action::None;
        }

        if (m_currentPage == MenuPage::Config) {
            if (m_selectedItem == getCurrentItemCount() - 1) {
                m_currentPage = MenuPage::Main;
                m_selectedItem = 0;
                return Action::Exit;
            }
            m_editMode = true;
            m_editItemIndex = m_selectedItem;
            if (m_onEnterEdit) {
                m_onEnterEdit(m_editItemIndex);
            }
            return Action::EnterSubmenu;
        }

        return Action::None;
    }

    Action cancel() {
        if (m_currentPage == MenuPage::Config) {
            m_currentPage = MenuPage::Main;
            m_selectedItem = 0;
            return Action::Exit;
        }
        if (m_currentPage == MenuPage::EditValue) {
            m_editMode = false;
            m_currentPage = MenuPage::Config;
            if (m_onExitEdit) {
                m_onExitEdit();
            }
            return Action::Exit;
        }
        return Action::None;
    }

    void setOnValueChange(void (*cb)(int)) { m_onValueChange = cb; }
    void setOnEnterEdit(void (*cb)(uint8_t)) { m_onEnterEdit = cb; }
    void setOnExitEdit(void (*cb)()) { m_onExitEdit = cb; }

    static const char* MAIN_ITEMS[];
    static const uint8_t MAIN_ITEM_COUNT = 1;

    static const char* CONFIG_ITEMS[];
    static const uint8_t CONFIG_ITEM_COUNT = 21;

private:
    MenuPage m_currentPage = MenuPage::Main;
    uint8_t m_selectedItem = 0;
    uint8_t m_editItemIndex = 0;
    bool m_editMode = false;

    void (*m_onValueChange)(int) = nullptr;
    void (*m_onEnterEdit)(uint8_t) = nullptr;
    void (*m_onExitEdit)() = nullptr;

    int getCurrentItemCount() const {
        switch (m_currentPage) {
            case MenuPage::Main:   return MAIN_ITEM_COUNT;
            case MenuPage::Config: return CONFIG_ITEM_COUNT;
            default:               return 0;
        }
    }
};

const char* MenuSystem::MAIN_ITEMS[] = {"Configuracion"};
const char* MenuSystem::CONFIG_ITEMS[] = {
    "Temp Offset", "Hum Offset", "Setpoint Temp",
    "Alarma Temp Alta", "Alarma Temp Baja",
    "Hum On Setpoint", "Hum Off Setpoint",
    "Alarma Hum Alta", "Alarma Hum Baja",
    "Intervalo Volteo", "Duracion Volteo",
    "Tipo Controlador", "Kp (PID)", "Ki (PID)", "Kd (PID)",
    "Histeresis (Hyst)", "b0 (LADRC)", "wc (LADRC)", "wo (LADRC)",
    "Reset Dias", "Volver"
};
