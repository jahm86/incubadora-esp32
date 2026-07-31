#include "display/MenuSystem.h"

namespace {

struct MenuEntry {
    const char* label;
    int8_t submenu;
    int8_t field;
    uint8_t action;
    bool back;
};

struct MenuNode {
    const char* title;
    int8_t parent;
    const MenuEntry* items;
    uint8_t count;
};

constexpr uint8_t NODE_ROOT    = 0;
constexpr uint8_t NODE_CONTROL = 1;
constexpr uint8_t NODE_TEMP    = 2;
constexpr uint8_t NODE_HUM     = 3;
constexpr uint8_t NODE_TURN    = 4;
constexpr uint8_t NODE_SYSTEM  = 5;

#define FIELD(f) static_cast<int8_t>(f)

const MenuEntry kRootItems[] = {
    {"Control",     NODE_CONTROL, -1, 0, false},
    {"Temperatura", NODE_TEMP,    -1, 0, false},
    {"Humedad",     NODE_HUM,     -1, 0, false},
    {"Volteo",      NODE_TURN,    -1, 0, false},
    {"Sistema",     NODE_SYSTEM,  -1, 0, false},
    {"Salir",       -1,           -1, 0, true},
};

const MenuEntry kControlItems[] = {
    {"Setpoint",    -1, FIELD(MenuField::Setpoint),      0, false},
    {"Controlador", -1, FIELD(MenuField::ControllerType), 0, false},
    {"Kp (PID)",    -1, FIELD(MenuField::Kp),             0, false},
    {"Ki (PID)",    -1, FIELD(MenuField::Ki),             0, false},
    {"Kd (PID)",    -1, FIELD(MenuField::Kd),             0, false},
    {"Histeresis",  -1, FIELD(MenuField::Hysteresis),     0, false},
    {"b0 (LADRC)",  -1, FIELD(MenuField::B0Coeff),    0, false},
    {"wc (LADRC)",  -1, FIELD(MenuField::Wc),             0, false},
    {"wo (LADRC)",  -1, FIELD(MenuField::Wo),             0, false},
    {"Volver",      -1, -1, 0, true},
};

const MenuEntry kTempItems[] = {
    {"Offset Temp",   -1, FIELD(MenuField::TempOffset),    0, false},
    {"Alarma T Alta", -1, FIELD(MenuField::TempAlarmHigh), 0, false},
    {"Alarma T Baja", -1, FIELD(MenuField::TempAlarmLow),  0, false},
    {"Volver",        -1, -1, 0, true},
};

const MenuEntry kHumItems[] = {
    {"Offset Hum",    -1, FIELD(MenuField::HumOffset),    0, false},
    {"Hum On",        -1, FIELD(MenuField::HumOn),        0, false},
    {"Hum Off",       -1, FIELD(MenuField::HumOff),       0, false},
    {"Alarma H Alta", -1, FIELD(MenuField::HumAlarmHigh), 0, false},
    {"Alarma H Baja", -1, FIELD(MenuField::HumAlarmLow),  0, false},
    {"Volver",        -1, -1, 0, true},
};

const MenuEntry kTurnItems[] = {
    {"Intervalo", -1, FIELD(MenuField::TurnInterval), 0, false},
    {"Duracion",  -1, FIELD(MenuField::TurnDuration), 0, false},
    {"Volver",    -1, -1, 0, true},
};

const MenuEntry kSystemItems[] = {
    {"Reset Dias", -1, -1, 1, false},
    {"Volver",     -1, -1, 0, true},
};

const MenuNode kNodes[] = {
    {"Menu",      -1,           kRootItems,   sizeof(kRootItems) / sizeof(MenuEntry)},
    {"Control",   NODE_ROOT,    kControlItems, sizeof(kControlItems) / sizeof(MenuEntry)},
    {"Temperatura", NODE_ROOT,  kTempItems,   sizeof(kTempItems) / sizeof(MenuEntry)},
    {"Humedad",   NODE_ROOT,    kHumItems,    sizeof(kHumItems) / sizeof(MenuEntry)},
    {"Volteo",    NODE_ROOT,    kTurnItems,   sizeof(kTurnItems) / sizeof(MenuEntry)},
    {"Sistema",   NODE_ROOT,    kSystemItems, sizeof(kSystemItems) / sizeof(MenuEntry)},
};

#undef FIELD

} // namespace

void MenuSystem::begin() {
    m_page = MenuPage::Main;
    m_node = NODE_ROOT;
    m_selected = 0;
    m_scroll = 0;
    m_editing = false;
}

void MenuSystem::openMenu() {
    m_page = MenuPage::Menu;
    m_node = NODE_ROOT;
    m_selected = 0;
    m_scroll = 0;
}

const char* MenuSystem::currentTitle() const {
    return kNodes[m_node].title;
}

uint8_t MenuSystem::currentItemCount() const {
    return kNodes[m_node].count;
}

const char* MenuSystem::itemLabel(uint8_t index) const {
    if (index >= kNodes[m_node].count) {
        return "";
    }
    return kNodes[m_node].items[index].label;
}

void MenuSystem::updateScroll() {
    uint8_t count = kNodes[m_node].count;
    if (m_selected < m_scroll) {
        m_scroll = m_selected;
    } else if (m_selected >= m_scroll + VISIBLE_ROWS) {
        m_scroll = m_selected - VISIBLE_ROWS + 1;
    }
    if (m_scroll + VISIBLE_ROWS > count) {
        m_scroll = count > VISIBLE_ROWS ? count - VISIBLE_ROWS : 0;
    }
}

void MenuSystem::navigate(int delta) {
    if (m_page == MenuPage::EditValue) {
        if (m_onValueChange) {
            m_onValueChange(delta);
        }
        return;
    }
    if (m_page != MenuPage::Menu) {
        return;
    }

    uint8_t count = kNodes[m_node].count;
    if (count == 0) {
        return;
    }
    int newSel = static_cast<int>(m_selected) + delta;
    if (newSel < 0) newSel = count - 1;
    if (newSel >= count) newSel = 0;
    m_selected = newSel;
    updateScroll();
}

void MenuSystem::confirm() {
    if (m_page != MenuPage::Menu) {
        return;
    }

    const MenuEntry& e = kNodes[m_node].items[m_selected];
    if (e.submenu >= 0) {
        m_node = e.submenu;
        m_selected = 0;
        m_scroll = 0;
    } else if (e.field >= 0) {
        m_editing = true;
        m_editField = static_cast<MenuField>(e.field);
        m_page = MenuPage::EditValue;
        if (m_onEnterEdit) {
            m_onEnterEdit(m_editField);
        }
    } else if (e.action > 0) {
        if (m_onAction) {
            m_onAction(e.action);
        }
    } else if (e.back) {
        cancel();
    }
}

void MenuSystem::cancel() {
    if (m_page == MenuPage::EditValue) {
        m_editing = false;
        m_page = MenuPage::Menu;
        if (m_onExitEdit) {
            m_onExitEdit(m_editField);
        }
        return;
    }
    if (m_page != MenuPage::Menu) {
        return;
    }

    int8_t parent = kNodes[m_node].parent;
    if (parent >= 0) {
        m_node = parent;
        m_selected = 0;
        m_scroll = 0;
    } else {
        m_page = MenuPage::Main;
    }
}
