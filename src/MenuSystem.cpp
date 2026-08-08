#include "display/MenuSystem.h"

namespace {

enum class Kind : uint8_t {
    Field,
    Submenu,
    Action,
    Info,
    Back,
    Confirm
};

struct MenuEntry {
    const char* label;
    Kind kind;
    int8_t target;
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

#define INFO_ID_NET  1
#define INFO_ID_MQTT 2

constexpr int8_t ACT_RESET_DAYS    = 1;
constexpr int8_t ACT_TOGGLE_WEB    = 2;
constexpr int8_t ACT_FACTORY_RESET = 3;
constexpr int8_t ACT_TURN_NOW      = 4;
constexpr int8_t ACT_TOGGLE_BUZZER = 5;
constexpr int8_t ACT_RESTART       = 6;

#define FIELD(f) static_cast<int8_t>(f)

const MenuEntry kRootItems[] = {
    {"Control",     Kind::Submenu, NODE_CONTROL},
    {"Temperatura", Kind::Submenu, NODE_TEMP},
    {"Humedad",     Kind::Submenu, NODE_HUM},
    {"Volteo",      Kind::Submenu, NODE_TURN},
    {"Sistema",     Kind::Submenu, NODE_SYSTEM},
    {"Salir",       Kind::Back,    -1},
};

const MenuEntry kControlItems[] = {
    {"Setpoint",    Kind::Field, FIELD(MenuField::Setpoint)},
    {"Controlador", Kind::Field, FIELD(MenuField::ControllerType)},
    {"Kp (PID)",    Kind::Field, FIELD(MenuField::Kp)},
    {"Ki (PID)",    Kind::Field, FIELD(MenuField::Ki)},
    {"Kd (PID)",    Kind::Field, FIELD(MenuField::Kd)},
    {"Histeresis",  Kind::Field, FIELD(MenuField::Hysteresis)},
    {"b0 (LADRC)",  Kind::Field, FIELD(MenuField::B0Coeff)},
    {"wc (LADRC)",  Kind::Field, FIELD(MenuField::Wc)},
    {"wo (LADRC)",  Kind::Field, FIELD(MenuField::Wo)},
    {"Volver",      Kind::Back,   -1},
};

const MenuEntry kTempItems[] = {
    {"Offset Temp",   Kind::Field, FIELD(MenuField::TempOffset)},
    {"Alarma T Alta", Kind::Field, FIELD(MenuField::TempAlarmHigh)},
    {"Alarma T Baja", Kind::Field, FIELD(MenuField::TempAlarmLow)},
    {"Volver",        Kind::Back,  -1},
};

const MenuEntry kHumItems[] = {
    {"Offset Hum",    Kind::Field, FIELD(MenuField::HumOffset)},
    {"Hum On",        Kind::Field, FIELD(MenuField::HumOn)},
    {"Hum Off",       Kind::Field, FIELD(MenuField::HumOff)},
    {"Alarma H Alta", Kind::Field, FIELD(MenuField::HumAlarmHigh)},
    {"Alarma H Baja", Kind::Field, FIELD(MenuField::HumAlarmLow)},
    {"Volver",        Kind::Back,  -1},
};

const MenuEntry kTurnItems[] = {
    {"Intervalo",     Kind::Field,   FIELD(MenuField::TurnInterval)},
    {"Duracion",      Kind::Field,   FIELD(MenuField::TurnDuration)},
    {"Pantalla Volteo", Kind::Field, FIELD(MenuField::TurnDisplayMode)},
    {"Voltear Ahora", Kind::Confirm, ACT_TURN_NOW},
    {"Volver",        Kind::Back,    -1},
};

const MenuEntry kSystemItems[] = {
    {"Servidor Web",   Kind::Action,  ACT_TOGGLE_WEB},
    {"Ver IP / Modo",  Kind::Info,    INFO_ID_NET},
    {"Estado MQTT",    Kind::Info,    INFO_ID_MQTT},
    {"Buzzer",         Kind::Action,  ACT_TOGGLE_BUZZER},
    {"Reiniciar",      Kind::Confirm, ACT_RESTART},
    {"Reset Dias",     Kind::Confirm, ACT_RESET_DAYS},
    {"Restaurar Fab.", Kind::Confirm, ACT_FACTORY_RESET},
    {"Volver",         Kind::Back,    -1},
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
    m_info = 0;
    m_confirmChoice = false;
    m_pendingAction = 0;
    m_hasDynLabel = false;
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
    if (m_hasDynLabel && m_dynNode == m_node && m_dynIndex == index) {
        return m_dynText;
    }
    return kNodes[m_node].items[index].label;
}

void MenuSystem::setDynamicLabel(uint8_t node, uint8_t index, const char* text) {
    m_dynNode = node;
    m_dynIndex = index;
    m_hasDynLabel = true;
    strncpy(m_dynText, text, sizeof(m_dynText) - 1);
    m_dynText[sizeof(m_dynText) - 1] = '\0';
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
    if (m_page == MenuPage::Confirm) {
        m_confirmChoice = !m_confirmChoice;
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
    if (m_page == MenuPage::Confirm) {
        if (m_confirmChoice) {
            if (m_onAction) {
                m_onAction(m_pendingAction);
            }
        }
        cancel();
        return;
    }
    if (m_page != MenuPage::Menu) {
        return;
    }

    const MenuEntry& e = kNodes[m_node].items[m_selected];
    switch (e.kind) {
        case Kind::Submenu:
            m_node = e.target;
            m_selected = 0;
            m_scroll = 0;
            break;
        case Kind::Field:
            m_editing = true;
            m_editField = static_cast<MenuField>(e.target);
            m_page = MenuPage::EditValue;
            if (m_onEnterEdit) {
                m_onEnterEdit(m_editField);
            }
            break;
        case Kind::Action:
            if (m_onAction) {
                m_onAction(e.target);
            }
            break;
        case Kind::Info:
            m_info = e.target;
            m_page = MenuPage::Info;
            break;
        case Kind::Confirm:
            m_pendingAction = e.target;
            m_confirmChoice = false;
            m_page = MenuPage::Confirm;
            break;
        case Kind::Back:
            cancel();
            break;
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
    if (m_page == MenuPage::Info || m_page == MenuPage::Confirm) {
        m_page = MenuPage::Menu;
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