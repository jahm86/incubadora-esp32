#include "config/FieldRegistry.h"
#include <math.h>
#include <string.h>

namespace {

const char* const kCtlNames[]      = {"Histeresis", "PID", "LADRC"};
const char* const kTurnModeNames[] = {"Desde (X)", "Hasta (Y)"};
const char* const kOnOffNames[]    = {"OFF", "ON"};

const FieldDef kDefinitions[static_cast<size_t>(MenuField::Count)] = {
    {
        MenuField::TempOffset, "temp_offset", "Offset Temp", "C",
        0.1f, -10.0f, 10.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.tempOffset; },
        [](SystemConfig& c, float v) { c.tempOffset = v; }
    },
    {
        MenuField::HumOffset, "hum_offset", "Offset Hum", "%",
        0.1f, -20.0f, 20.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.humOffset; },
        [](SystemConfig& c, float v) { c.humOffset = v; }
    },
    {
        MenuField::Setpoint, "setpoint", "Setpoint", "C",
        0.1f, 20.0f, 60.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.setpoint; },
        [](SystemConfig& c, float v) { c.setpoint = v; }
    },
    {
        MenuField::TempAlarmHigh, "temp_alarm_high", "Alarma T Alta", "C",
        0.1f, 0.0f, 60.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.tempAlarmHigh; },
        [](SystemConfig& c, float v) { c.tempAlarmHigh = v; }
    },
    {
        MenuField::TempAlarmLow, "temp_alarm_low", "Alarma T Baja", "C",
        0.1f, 0.0f, 60.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.tempAlarmLow; },
        [](SystemConfig& c, float v) { c.tempAlarmLow = v; }
    },
    {
        MenuField::HumOn, "hum_on", "Hum On", "%",
        0.5f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.humSetpointOn; },
        [](SystemConfig& c, float v) { c.humSetpointOn = v; }
    },
    {
        MenuField::HumOff, "hum_off", "Hum Off", "%",
        0.5f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.humSetpointOff; },
        [](SystemConfig& c, float v) { c.humSetpointOff = v; }
    },
    {
        MenuField::HumAlarmHigh, "hum_alarm_high", "Alarma H Alta", "%",
        0.5f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.humAlarmHigh; },
        [](SystemConfig& c, float v) { c.humAlarmHigh = v; }
    },
    {
        MenuField::HumAlarmLow, "hum_alarm_low", "Alarma H Baja", "%",
        0.5f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.humAlarmLow; },
        [](SystemConfig& c, float v) { c.humAlarmLow = v; }
    },
    {
        MenuField::TurnInterval, "turn_interval", "Intervalo Volteo", "min",
        1.0f, 1.0f, 1440.0f, true, false, nullptr,
        [](const SystemConfig& c) { return static_cast<float>(c.turnInterval); },
        [](SystemConfig& c, float v) { c.turnInterval = static_cast<uint32_t>(v); }
    },
    {
        MenuField::TurnDuration, "turn_duration", "Duracion Volteo", "s",
        1.0f, 1.0f, 60.0f, true, false, nullptr,
        [](const SystemConfig& c) { return static_cast<float>(c.turnDuration); },
        [](SystemConfig& c, float v) { c.turnDuration = static_cast<uint32_t>(v); }
    },
    {
        MenuField::TurnDisplayMode, "turn_display_mode", "Pantalla Volteo", "",
        1.0f, 0.0f, 1.0f, true, true, kTurnModeNames,
        [](const SystemConfig& c) { return static_cast<float>(c.turnDisplayMode); },
        [](SystemConfig& c, float v) { c.turnDisplayMode = static_cast<uint8_t>(v); }
    },
    {
        MenuField::ControllerType, "controller_type", "Controlador", "",
        1.0f, 0.0f, 2.0f, true, true, kCtlNames,
        [](const SystemConfig& c) { return static_cast<float>(c.controllerType); },
        [](SystemConfig& c, float v) { c.controllerType = static_cast<uint8_t>(v); }
    },
    {
        MenuField::Kp, "kp", "Kp (PID)", "",
        1.0f, 0.0f, 1000.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.kp; },
        [](SystemConfig& c, float v) { c.kp = v; }
    },
    {
        MenuField::Ki, "ki", "Ki (PID)", "",
        0.1f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.ki; },
        [](SystemConfig& c, float v) { c.ki = v; }
    },
    {
        MenuField::Kd, "kd", "Kd (PID)", "",
        1.0f, 0.0f, 100.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.kd; },
        [](SystemConfig& c, float v) { c.kd = v; }
    },
    {
        MenuField::Hysteresis, "hysteresis", "Histeresis", "C",
        0.1f, 0.1f, 5.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.hysteresis; },
        [](SystemConfig& c, float v) { c.hysteresis = v; }
    },
    {
        MenuField::B0Coeff, "b0", "b0 (LADRC)", "",
        1.0f, 1.0f, 1000.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.b0; },
        [](SystemConfig& c, float v) { c.b0 = v; }
    },
    {
        MenuField::Wc, "wc", "wc (LADRC)", "",
        1.0f, 0.1f, 500.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.wc; },
        [](SystemConfig& c, float v) { c.wc = v; }
    },
    {
        MenuField::Wo, "wo", "wo (LADRC)", "",
        1.0f, 0.1f, 500.0f, false, false, nullptr,
        [](const SystemConfig& c) { return c.wo; },
        [](SystemConfig& c, float v) { c.wo = v; }
    },
    {
        MenuField::WebEnabled, "", "Servidor Web", "",
        1.0f, 0.0f, 1.0f, true, true, kOnOffNames,
        [](const SystemConfig&) { return 0.0f; },
        [](SystemConfig&, float) {}
    },
    {
        MenuField::BuzzerEnabled, "buzzer_enabled", "Buzzer", "",
        1.0f, 0.0f, 1.0f, true, true, kOnOffNames,
        [](const SystemConfig& c) { return c.buzzerEnabled ? 1.0f : 0.0f; },
        [](SystemConfig& c, float v) { c.buzzerEnabled = (v >= 0.5f); }
    }
};

} // namespace

const FieldDef* FieldRegistry::get(MenuField field) {
    uint8_t idx = static_cast<uint8_t>(field);
    if (idx >= count()) {
        return nullptr;
    }
    return &kDefinitions[idx];
}

const FieldDef* FieldRegistry::findByKey(const char* key) {
    if (!key || key[0] == '\0') {
        return nullptr;
    }
    for (size_t i = 0; i < count(); i++) {
        if (kDefinitions[i].key && strcmp(kDefinitions[i].key, key) == 0) {
            return &kDefinitions[i];
        }
    }
    return nullptr;
}

const FieldDef* FieldRegistry::findByKey(const String& key) {
    return findByKey(key.c_str());
}

const FieldDef* FieldRegistry::all() {
    return kDefinitions;
}

float FieldRegistry::clampValue(const FieldDef& def, float value) {
    float v = constrain(value, def.min, def.max);
    if (def.isInt || def.isEnum) {
        v = roundf(v);
    }
    return v;
}

void FieldRegistry::formatValue(const FieldDef& def, float value, char* buf, size_t size) {
    if (def.isEnum && def.enumNames) {
        int idx = constrain(static_cast<int>(roundf(value)), 0, static_cast<int>(def.max));
        snprintf(buf, size, "%s", def.enumNames[idx]);
    } else if (def.isInt) {
        snprintf(buf, size, "%.0f %s", value, def.unit);
    } else {
        snprintf(buf, size, "%.1f %s", value, def.unit);
    }
}

void FieldRegistry::clampCrossDependencies(SystemConfig& cfg) {
    cfg.tempAlarmHigh  = constrain(cfg.tempAlarmHigh, cfg.setpoint, 60.0f);
    cfg.tempAlarmLow   = constrain(cfg.tempAlarmLow, 0.0f, cfg.setpoint);
    cfg.humSetpointOff = constrain(cfg.humSetpointOff, cfg.humSetpointOn, 100.0f);
}
