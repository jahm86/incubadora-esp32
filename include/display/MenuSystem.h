#pragma once

#include <Arduino.h>
#include <functional>
#include "config/settings.h"
#include "types.h"

enum class MenuField : uint8_t {
    TempOffset,
    HumOffset,
    Setpoint,
    TempAlarmHigh,
    TempAlarmLow,
    HumOn,
    HumOff,
    HumAlarmHigh,
    HumAlarmLow,
    TurnInterval,
    TurnDuration,
    ControllerType,
    Kp,
    Ki,
    Kd,
    Hysteresis,
    B0Coeff,
    Wc,
    Wo,
    Count
};

class MenuSystem {
public:
    using ValueChangeCallback = std::function<void(int)>;
    using EnterEditCallback   = std::function<void(MenuField)>;
    using ExitEditCallback    = std::function<void(MenuField)>;
    using ActionCallback      = std::function<void(uint8_t)>;

    void begin();
    void openMenu();

    MenuPage currentPage() const { return m_page; }
    bool isEditing() const { return m_editing; }
    uint8_t selectedItem() const { return m_selected; }
    uint8_t scrollOffset() const { return m_scroll; }
    MenuField editField() const { return m_editField; }

    const char* currentTitle() const;
    uint8_t currentItemCount() const;
    const char* itemLabel(uint8_t index) const;

    void navigate(int delta);
    void confirm();
    void cancel();

    void setOnValueChange(ValueChangeCallback cb) { m_onValueChange = cb; }
    void setOnEnterEdit(EnterEditCallback cb) { m_onEnterEdit = cb; }
    void setOnExitEdit(ExitEditCallback cb) { m_onExitEdit = cb; }
    void setOnAction(ActionCallback cb) { m_onAction = cb; }

    static constexpr uint8_t VISIBLE_ROWS = 9;

private:
    MenuPage m_page = MenuPage::Main;
    int8_t m_node = 0;
    uint8_t m_selected = 0;
    uint8_t m_scroll = 0;
    bool m_editing = false;
    MenuField m_editField = MenuField::Setpoint;

    ValueChangeCallback m_onValueChange;
    EnterEditCallback m_onEnterEdit;
    ExitEditCallback m_onExitEdit;
    ActionCallback m_onAction;

    void updateScroll();
};
