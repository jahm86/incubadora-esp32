#include <Arduino.h>
#include <math.h>
#include <esp_task_wdt.h>
#include "config/pins.h"
#include "config/settings.h"
#include "core/AppState.h"
#include "core/ConfigManager.h"
#include "sensor/AHT30.h"
#include "control/IController.h"
#include "control/PIDController.h"
#include "control/HysteresisController.h"
#include "control/LADRCController.h"
#include "display/DisplayManager.h"
#include "display/MenuSystem.h"
#include "input/RotaryEncoder.h"
#include "network/WiFiManager.h"
#include "network/MqttManager.h"
#include "web/WebServer.h"
#include "output/Buzzer.h"
#include "output/Heater.h"
#include "output/Humidifier.h"
#include "output/EggTray.h"

AppState          appState;
ConfigManager     configManager;
AHT30             sensor;
PIDController          pidController(Settings::DEFAULT_KP, Settings::DEFAULT_KI, Settings::DEFAULT_KD);
HysteresisController   hysteresisController(Settings::DEFAULT_HYSTERESIS);
LADRCController        ladrcController(Settings::DEFAULT_B0, Settings::DEFAULT_WC, Settings::DEFAULT_WO);
IController*      activeController = nullptr;
DisplayManager    display;
MenuSystem        menuSystem;
RotaryEncoderInput encoder;
WiFiManager       wifiManager;
MqttManager       mqttManager;
WebServerManager  webServer;
Buzzer            buzzer;
Heater            heater;
Humidifier        humidifier;
EggTray           eggTray;

unsigned long lastSensorRead      = 0;
unsigned long lastControl         = 0;
unsigned long lastMqttPublish     = 0;
unsigned long lastIncubationSave  = 0;
unsigned long lastTurnCheck       = 0;
unsigned long lastTurnTime        = 0;
unsigned long incubationStart     = 0;

bool g_turning = false;
unsigned long g_turnStart = 0;

volatile bool alarmActive = false;
volatile uint32_t alarmSnoozedUntil = 0;
char alarmMessage[64] = {0};
uint8_t alarmMask = 0;
const BufferTone* alarmPlayingBuf = nullptr;

const Tone kUiClick[] = {{4, 2}, {4, 1}};
const Tone kUiTick[]  = {{0, 4}};

const Tone kAlarmTempHigh[] = {{39, 19}, {9, 0}};
const Tone kAlarmTempLow[]  = {{44, 11}, {4, 0}};
const Tone kAlarmHumHigh[]  = {{4, 40}, {4, 25}, {4, 2}};
const Tone kAlarmHumLow[]   = {{4, 20}, {4, 15}, {4, 1}};

const BufferTone kAlarmTempHighBuf = {const_cast<Tone*>(kAlarmTempHigh), sizeof(kAlarmTempHigh) / sizeof(Tone), true};
const BufferTone kAlarmTempLowBuf  = {const_cast<Tone*>(kAlarmTempLow),  sizeof(kAlarmTempLow)  / sizeof(Tone), true};
const BufferTone kAlarmHumHighBuf  = {const_cast<Tone*>(kAlarmHumHigh),  sizeof(kAlarmHumHigh)  / sizeof(Tone), true};
const BufferTone kAlarmHumLowBuf   = {const_cast<Tone*>(kAlarmHumLow),   sizeof(kAlarmHumLow)   / sizeof(Tone), true};

#define ALARM_MASK_TEMP_LOW  (1 << 0)
#define ALARM_MASK_TEMP_HIGH (1 << 1)
#define ALARM_MASK_HUM_LOW   (1 << 2)
#define ALARM_MASK_HUM_HIGH  (1 << 3)

const BufferTone* alarmBufferForMask(uint8_t mask) {
    if (mask & ALARM_MASK_TEMP_HIGH) return &kAlarmTempHighBuf;
    if (mask & ALARM_MASK_TEMP_LOW)  return &kAlarmTempLowBuf;
    if (mask & ALARM_MASK_HUM_HIGH)  return &kAlarmHumHighBuf;
    return &kAlarmHumLowBuf;
}

void syncAndSaveConfig() {
    configManager.config() = appState.config();
    configManager.save();
}

void initController() {
    if (activeController) {
        activeController->reset();
    }

    float Ts = Settings::CONTROL_INTERVAL_MS / 1000.0f;

    switch (appState.controllerType()) {
        case Settings::ControllerType::PID:
            pidController.setTunings(appState.config().kp, appState.config().ki, appState.config().kd);
            pidController.setTs(Ts);
            pidController.setSetpoint(appState.config().setpoint);
            activeController = &pidController;
            break;
        case Settings::ControllerType::LADRC:
            ladrcController.setTunings(appState.config().b0, appState.config().wc, appState.config().wo);
            ladrcController.setTs(Ts);
            ladrcController.setSetpoint(appState.config().setpoint);
            activeController = &ladrcController;
            break;
        default:
            hysteresisController.setHysteresis(appState.config().hysteresis);
            hysteresisController.setTs(Ts);
            hysteresisController.setSetpoint(appState.config().setpoint);
            activeController = &hysteresisController;
            break;
    }
    activeController->begin();
}

void readSensor() {
    SensorData data = sensor.readWithOffset(
        appState.config().tempOffset,
        appState.config().humOffset);
    appState.sensor() = data;
}

void runControl() {
    if (!appState.sensor().valid) return;

    float temp = appState.sensor().temperature;

    float heaterPower = activeController->compute(temp);
    heater.setPower(heaterPower);
    appState.outputs().heaterPower = heaterPower;
    appState.outputs().heaterActive = heaterPower > 0.01f;

    float hum = appState.sensor().humidity;
    if (hum < appState.config().humSetpointOn) {
        humidifier.on();
        appState.outputs().humidifierActive = true;
    } else if (hum > appState.config().humSetpointOff) {
        humidifier.off();
        appState.outputs().humidifierActive = false;
    }
}

void checkAlarms() {
    uint8_t mask = 0;
    char msg[64] = {0};

    if (appState.sensor().valid) {
        float temp = appState.sensor().temperature;
        float hum  = appState.sensor().humidity;

        if (temp > appState.config().tempAlarmHigh) {
            mask |= ALARM_MASK_TEMP_HIGH;
            snprintf(msg, sizeof(msg), "Temp ALTA: %.1f C", temp);
        } else if (temp < appState.config().tempAlarmLow) {
            mask |= ALARM_MASK_TEMP_LOW;
            snprintf(msg, sizeof(msg), "Temp BAJA: %.1f C", temp);
        }

        if (hum > appState.config().humAlarmHigh) {
            mask |= ALARM_MASK_HUM_HIGH;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum ALTA: %.1f%%", hum);
        } else if (hum < appState.config().humAlarmLow) {
            mask |= ALARM_MASK_HUM_LOW;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum BAJA: %.1f%%", hum);
        }
    }

    bool shouldAlarm = (mask != 0) && (millis() >= alarmSnoozedUntil);

    if (shouldAlarm && !alarmActive) {
        alarmActive = true;
        alarmMask = mask;
        strncpy(alarmMessage, msg, sizeof(alarmMessage) - 1);
        alarmMessage[sizeof(alarmMessage) - 1] = '\0';
        appState.outputs().buzzerActive = true;
        alarmPlayingBuf = alarmBufferForMask(mask);
        buzzer.play(*alarmPlayingBuf);
        if (mqttManager.isConnected()) {
            mqttManager.publish(MqttTopics::ALARM, alarmMessage);
        }
    } else if (shouldAlarm && alarmActive) {
        const BufferTone* buf = alarmBufferForMask(mask);
        if (buf != alarmPlayingBuf) {
            alarmPlayingBuf = buf;
            buzzer.play(*buf);
        }
        if (mask != alarmMask) {
            alarmMask = mask;
            strncpy(alarmMessage, msg, sizeof(alarmMessage) - 1);
            alarmMessage[sizeof(alarmMessage) - 1] = '\0';
        }
    } else if (!shouldAlarm && alarmActive) {
        alarmActive = false;
        alarmMask = 0;
        alarmPlayingBuf = nullptr;
        appState.outputs().buzzerActive = false;
        buzzer.stop();
    }
}

void startTurn() {
    eggTray.on();
    appState.outputs().eggTrayActive = true;
    g_turning = true;
    g_turnStart = millis();
    lastTurnTime = g_turnStart;
    lastTurnCheck = g_turnStart;
    log_i("Egg turn started now");
}

void checkEggTurn() {
    if (g_turning) {
        if (millis() - g_turnStart > appState.config().turnDuration * 1000UL) {
            eggTray.off();
            appState.outputs().eggTrayActive = false;
            g_turning = false;
        }
        return;
    }

    if (millis() - lastTurnCheck > appState.config().turnInterval * 60UL * 1000UL) {
        startTurn();
    }
}

void updateIncubationDays() {
    if (incubationStart > 0) {
        uint32_t elapsed = (millis() - incubationStart) / 86400000UL;
        if (elapsed > 0) {
            configManager.setIncubationDays(elapsed);
            appState.setIncubationDays(elapsed);
        }
    }
}

bool g_webRunning = false;

static float g_editOriginal = 0.0f;
static float g_editCurrent = 0.0f;

struct ConfigFieldDef {
    const char* key;
    const char* label;
    const char* unit;
    std::function<float()> get;
    std::function<void(float)> set;
    float step;
    float min;
    float max;
    bool isInt;
    bool isEnum;
    const char* const* enumNames;
};

static const ConfigFieldDef* fieldDefs() {
    static ConfigFieldDef defs[static_cast<uint8_t>(MenuField::Count)];
    static bool init = false;
    if (init) {
        return defs;
    }
    init = true;

    static const char* ctlNames[]     = {"Histeresis", "PID", "LADRC"};
    static const char* turnModeNames[] = {"Desde (X)", "Hasta (Y)"};
    static const char* onOffNames[]   = {"OFF", "ON"};

    auto* P = &appState.config();

    defs[static_cast<uint8_t>(MenuField::TempOffset)] =
        {"temp_offset", "Offset Temp", "C", [P] { return P->tempOffset; }, [P](float v) { P->tempOffset = v; },
         0.1f, -10.0f, 10.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::HumOffset)] =
        {"hum_offset", "Offset Hum", "%", [P] { return P->humOffset; }, [P](float v) { P->humOffset = v; },
         0.1f, -20.0f, 20.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Setpoint)] =
        {"setpoint", "Setpoint", "C", [P] { return P->setpoint; }, [P](float v) { P->setpoint = v; },
         0.1f, 20.0f, 60.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::TempAlarmHigh)] =
        {"temp_alarm_high", "Alarma T Alta", "C", [P] { return P->tempAlarmHigh; }, [P](float v) { P->tempAlarmHigh = v; },
         0.1f, 0.0f, 60.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::TempAlarmLow)] =
        {"temp_alarm_low", "Alarma T Baja", "C", [P] { return P->tempAlarmLow; }, [P](float v) { P->tempAlarmLow = v; },
         0.1f, 0.0f, 60.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::HumOn)] =
        {"hum_on", "Hum On", "%", [P] { return P->humSetpointOn; }, [P](float v) { P->humSetpointOn = v; },
         0.5f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::HumOff)] =
        {"hum_off", "Hum Off", "%", [P] { return P->humSetpointOff; }, [P](float v) { P->humSetpointOff = v; },
         0.5f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::HumAlarmHigh)] =
        {"hum_alarm_high", "Alarma H Alta", "%", [P] { return P->humAlarmHigh; }, [P](float v) { P->humAlarmHigh = v; },
         0.5f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::HumAlarmLow)] =
        {"hum_alarm_low", "Alarma H Baja", "%", [P] { return P->humAlarmLow; }, [P](float v) { P->humAlarmLow = v; },
         0.5f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::TurnInterval)] =
        {"turn_interval", "Intervalo Volteo", "min", [P] { return static_cast<float>(P->turnInterval); },
         [P](float v) { P->turnInterval = static_cast<uint32_t>(v); },
         1.0f, 1.0f, 1440.0f, true, false};
    defs[static_cast<uint8_t>(MenuField::TurnDuration)] =
        {"turn_duration", "Duracion Volteo", "s", [P] { return static_cast<float>(P->turnDuration); },
         [P](float v) { P->turnDuration = static_cast<uint32_t>(v); },
         1.0f, 1.0f, 60.0f, true, false, nullptr};
    defs[static_cast<uint8_t>(MenuField::TurnDisplayMode)] =
        {"turn_display_mode", "Pantalla Volteo", "", [P] { return static_cast<float>(P->turnDisplayMode); },
         [P](float v) { P->turnDisplayMode = static_cast<uint8_t>(v); },
         1.0f, 0.0f, 1.0f, true, true, turnModeNames};
    defs[static_cast<uint8_t>(MenuField::ControllerType)] =
        {"controller_type", "Controlador", "", [P] { return static_cast<float>(P->controllerType); },
         [P](float v) { P->controllerType = static_cast<uint8_t>(v); },
         1.0f, 0.0f, 2.0f, true, true, ctlNames};
    defs[static_cast<uint8_t>(MenuField::Kp)] =
        {"kp", "Kp (PID)", "", [P] { return P->kp; }, [P](float v) { P->kp = v; },
         1.0f, 0.0f, 1000.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Ki)] =
        {"ki", "Ki (PID)", "", [P] { return P->ki; }, [P](float v) { P->ki = v; },
         0.1f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Kd)] =
        {"kd", "Kd (PID)", "", [P] { return P->kd; }, [P](float v) { P->kd = v; },
         1.0f, 0.0f, 100.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Hysteresis)] =
        {"hysteresis", "Histeresis", "C", [P] { return P->hysteresis; }, [P](float v) { P->hysteresis = v; },
         0.1f, 0.1f, 5.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::B0Coeff)] =
        {"b0", "b0 (LADRC)", "", [P] { return P->b0; }, [P](float v) { P->b0 = v; },
         1.0f, 1.0f, 1000.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Wc)] =
        {"wc", "wc (LADRC)", "", [P] { return P->wc; }, [P](float v) { P->wc = v; },
         1.0f, 0.1f, 500.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::Wo)] =
        {"wo", "wo (LADRC)", "", [P] { return P->wo; }, [P](float v) { P->wo = v; },
         1.0f, 0.1f, 500.0f, false, false};
    defs[static_cast<uint8_t>(MenuField::WebEnabled)] =
        {"", "Servidor Web", "", [] { return g_webRunning ? 1.0f : 0.0f; },
         [](float v) {
             bool on = v >= 0.5f;
             if (on != g_webRunning) {
                 g_webRunning = on;
                 if (on) {
                     webServer.start(&configManager);
                     log_i("Web server ON");
                 } else {
                     webServer.stop();
                     log_i("Web server OFF");
                 }
             }
         },
         1.0f, 0.0f, 1.0f, true, true, onOffNames};
    defs[static_cast<uint8_t>(MenuField::BuzzerEnabled)] =
        {"", "Buzzer", "", [P] { return P->buzzerEnabled ? 1.0f : 0.0f; },
         [P](float v) {
             bool on = v >= 0.5f;
             P->buzzerEnabled = on;
             buzzer.setEnabled(on);
         },
         1.0f, 0.0f, 1.0f, true, true, onOffNames};

    return defs;
}

constexpr uint8_t INFO_ID_NET  = 1;
constexpr uint8_t INFO_ID_MQTT = 2;

void formatFieldValue(const ConfigFieldDef& def, float value, char* buf, size_t size) {
    if (def.isEnum) {
        int idx = constrain(static_cast<int>(value), 0, static_cast<int>(def.max));
        snprintf(buf, size, "%s", def.enumNames[idx]);
    } else if (def.isInt) {
        snprintf(buf, size, "%.0f %s", value, def.unit);
    } else {
        snprintf(buf, size, "%.1f %s", value, def.unit);
    }
}

void onFieldValueChange(int delta) {
    (void)delta;
    MenuField f = menuSystem.editField();
    const ConfigFieldDef& def = fieldDefs()[static_cast<uint8_t>(f)];

    long pos = encoder.getValue();
    float v = def.min + static_cast<float>(pos) * def.step;
    if (def.isEnum || def.isInt) {
        v = roundf(v);
    }
    g_editCurrent = constrain(v, def.min, def.max);
}

void onEnterEdit(MenuField f) {
    const ConfigFieldDef& def = fieldDefs()[static_cast<uint8_t>(f)];
    g_editOriginal = def.get();
    long maxPos = lround((def.max - def.min) / def.step);
    encoder.setBoundaries(0, maxPos, false);
    long pos = lround((g_editOriginal - def.min) / def.step);
    encoder.setValue(pos);
    g_editCurrent = g_editOriginal;
}

void onExitEdit(MenuField f) {
    encoder.setBoundaries(0, 255, true);
    encoder.setValue(0);

    if (fabsf(g_editCurrent - g_editOriginal) > 1e-4f) {
        const ConfigFieldDef& def = fieldDefs()[static_cast<uint8_t>(f)];
        def.set(g_editCurrent);
        if (f == MenuField::Setpoint || f == MenuField::ControllerType) {
            initController();
        }
        syncAndSaveConfig();
        log_i("Edit commit: %s = %g", def.label, (double)g_editCurrent);
    } else {
        log_i("Edit exit sin cambios");
    }
}

void factoryReset() {
    configManager.factoryReset();
    log_w("System factory reset requested, rebooting");
    delay(100);
    ESP.restart();
}

void onMenuAction(uint8_t action) {
    if (action == 1) {
        configManager.setIncubationDays(0);
        configManager.setIncubationElapsedS(0);
        appState.setIncubationDays(0);
        incubationStart = millis();
        syncAndSaveConfig();
        log_i("Incubation days reset");
    } else if (action == 3) {
        factoryReset();
    } else if (action == 4) {
        startTurn();
    } else if (action == 6) {
        log_i("Restart requested via menu");
        delay(100);
        ESP.restart();
    }
}

const ConfigFieldDef* findFieldByKey(const String& key) {
    const ConfigFieldDef* defs = fieldDefs();
    for (uint8_t i = 0; i < static_cast<uint8_t>(MenuField::Count); i++) {
        if (strcmp(defs[i].key, key.c_str()) == 0) {
            return &defs[i];
        }
    }
    return nullptr;
}

void applyMqttSet(const String& key, const String& payload) {
    const ConfigFieldDef* def = findFieldByKey(key);
    if (!def) {
        log_d("MQTT: unknown config param %s", key.c_str());
        return;
    }
    float v = payload.toFloat();
    v = constrain(v, def->min, def->max);
    if (def->isInt || def->isEnum) {
        v = roundf(v);
    }
    def->set(v);
    if (key == "setpoint" || key == "controller_type") {
        initController();
    }
    log_i("MQTT set: %s = %g", key.c_str(), (double)v);
    syncAndSaveConfig();
}

void handleMqttRequest(const String& payload) {
    String name = payload;
    name.trim();
    const ConfigFieldDef* def = findFieldByKey(name);
    char resp[48];
    if (!def) {
        snprintf(resp, sizeof(resp), "error(desconocido):%s", name.c_str());
        mqttManager.publish(MqttTopics::CONFIG_RESPONSE, resp);
        return;
    }
    if (def->isInt || def->isEnum) {
        snprintf(resp, sizeof(resp), "%s:%d", name.c_str(), static_cast<int>(roundf(def->get())));
    } else {
        snprintf(resp, sizeof(resp), "%s:%.1f", name.c_str(), def->get());
    }
    mqttManager.publish(MqttTopics::CONFIG_RESPONSE, resp);
}

#if TONE_TEST_ON
static Tone bf_tone[64];
static uint8_t len_tone = 0;
static bool repeat_tone = false;

static int parseToneNum(const char* s, size_t n) {
    if (n == 0 || n > 3) return -1;
    int v = 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        v = v * 10 + (s[i] - '0');
    }
    return (v <= 255) ? v : -1;
}

void handleTestTone(const String& payload) {
    const char* p = payload.c_str();
    const size_t len = payload.length();

    if (len < 5 || p[0] != '(') {
        log_e("Tone test: formato invalido, se esperaba '(' en la pos 0");
        return;
    }
    const char* close = strchr(p, ')');
    if (!close) {
        log_e("Tone test: falta el ')' en el payload");
        return;
    }

    uint8_t count = 0;
    bool ok = true;
    const char* cur = p + 1;

    while (cur < close) {
        const size_t pos = static_cast<size_t>(cur - p);
        const char* comma = strchr(cur, ',');
        if (!comma || comma >= close) {
            log_e("Tone test: pos %u: falta la ',' entre tiempo y frecuencia", (unsigned)pos);
            ok = false;
            break;
        }
        const int time = parseToneNum(cur, static_cast<size_t>(comma - cur));
        if (time < 0) {
            log_e("Tone test: pos %u: tiempo invalido (0-255)", (unsigned)pos);
            ok = false;
            break;
        }
        const char* semi = strchr(comma + 1, ';');
        const char* end = (semi && semi < close) ? semi : close;
        const int freq = parseToneNum(comma + 1, static_cast<size_t>(end - comma - 1));
        if (freq < 0) {
            log_e("Tone test: pos %u: frecuencia invalida (0-255)", (unsigned)pos);
            ok = false;
            break;
        }
        if (count >= 64) {
            log_e("Tone test: pos %u: maximo 64 tonos", (unsigned)pos);
            ok = false;
            break;
        }
        bf_tone[count].time = static_cast<uint8_t>(time);
        bf_tone[count].frequency = static_cast<uint8_t>(freq);
        count++;
        cur = end + 1;
    }

    if (!ok) {
        return;
    }
    if (count == 0) {
        log_e("Tone test: sin tonos en el payload");
        return;
    }

    len_tone = count;
    repeat_tone = (close[1] == 't');
    buzzer.playNonBlocking(bf_tone, len_tone, repeat_tone);
    log_i("Tone test: %u tonos, repeat=%s", len_tone, repeat_tone ? "t" : "f");
}
#endif

void uiTask(void* param) {
    (void)param;
    esp_task_wdt_add(NULL);

    uint32_t lastDisplay = 0;
    int lastEncVal = encoder.getValue();
    bool wasEditing = menuSystem.isEditing();

    while (true) {
        esp_task_wdt_reset();
        encoder.loop();

        if (menuSystem.isEditing() != wasEditing) {
            wasEditing = menuSystem.isEditing();
            lastEncVal = encoder.getValue();
        }

        int btn = encoder.pollButton();
        if (btn == 1) {
            if (!alarmActive) {
                buzzer.playNonBlocking(kUiClick, sizeof(kUiClick) / sizeof(Tone), false);
            }
        } else if (btn == -1) {
            if (alarmActive) {
                alarmSnoozedUntil = millis() + Settings::ALARM_SNOOZE_MS;
                alarmActive = false;
                appState.outputs().buzzerActive = false;
                buzzer.stop();
                log_i("Alarm snoozed 10 min");
            } else if (menuSystem.isEditing()) {
                menuSystem.cancel();
            } else if (menuSystem.currentPage() == MenuPage::Main) {
                menuSystem.openMenu();
            } else if (menuSystem.currentPage() == MenuPage::Info) {
                menuSystem.cancel();
            } else {
                menuSystem.confirm();
            }
        }

        if (menuSystem.isEditing() != wasEditing) {
            wasEditing = menuSystem.isEditing();
            lastEncVal = encoder.getValue();
        }

        int encVal = encoder.getValue();
        if (encVal != lastEncVal) {
            int delta = encVal - lastEncVal;
            if (delta > 127) {
                delta -= 256;
            } else if (delta < -127) {
                delta += 256;
            }
            lastEncVal = encVal;
            if (!alarmActive) {
                buzzer.playNonBlocking(kUiTick, sizeof(kUiTick) / sizeof(Tone), false);
            }
            menuSystem.navigate(delta);
        }

        uint32_t now = millis();
        if (now - lastDisplay >= Settings::DISPLAY_REFRESH_MS) {
            lastDisplay = now;

            if (alarmActive) {
                display.drawAlarm(alarmMessage);
            } else if (menuSystem.isEditing()) {
                const ConfigFieldDef& def = fieldDefs()[static_cast<uint8_t>(menuSystem.editField())];
                char valBuf[24];
                formatFieldValue(def, g_editCurrent, valBuf, sizeof(valBuf));
                display.drawEditValue(def.label, valBuf);
            } else if (menuSystem.currentPage() == MenuPage::Info) {
                if (menuSystem.infoId() == INFO_ID_MQTT) {
                    char line1[24], line2[24];
                    snprintf(line1, sizeof(line1), "%s", mqttManager.stateText());
                    if (appState.config().mqttServer.length() > 0) {
                        snprintf(line2, sizeof(line2), "Broker: %s", appState.config().mqttServer.c_str());
                    } else {
                        snprintf(line2, sizeof(line2), "Pendiente config");
                    }
                    display.drawInfo("MQTT", line1, line2);
                } else {
                    char line1[16], line2[24];
                    if (appState.isApMode()) {
                        snprintf(line1, sizeof(line1), "Modo: AP");
                    } else {
                        snprintf(line1, sizeof(line1), "Modo: STA");
                    }
                    if (appState.infoIp().length() > 0) {
                        snprintf(line2, sizeof(line2), "IP: %s", appState.infoIp().c_str());
                    } else {
                        snprintf(line2, sizeof(line2), "IP: No obtenida");
                    }
                    display.drawInfo("Red", line1, line2);
                }
            } else if (menuSystem.currentPage() == MenuPage::Confirm) {
                const char* title;
                const char* line;
                if (menuSystem.pendingAction() == 1) {
                    title = "Reset Dias";
                    line = "Reset diario?";
                } else if (menuSystem.pendingAction() == 4) {
                    title = "Volteo";
                    line = "Voltear ahora?";
                } else if (menuSystem.pendingAction() == 6) {
                    title = "Reiniciar";
                    line = "Reiniciar sistema?";
                } else {
                    title = "Restaurar Fab.";
                    line = "Borrar config?";
                }
                display.drawConfirm(title, line, menuSystem.confirmChoice());
            } else if (menuSystem.currentPage() == MenuPage::Menu) {
                display.drawMenu(menuSystem);
            } else {
                uint32_t nowMs = millis();
                uint32_t sinceMin = (nowMs - lastTurnTime) / 60000UL;
                bool until = appState.config().turnDisplayMode == 1;
                uint32_t turnMin;
                if (until) {
                    uint32_t interval = appState.config().turnInterval;
                    turnMin = sinceMin >= interval ? 0 : interval - sinceMin;
                } else {
                    turnMin = sinceMin;
                }
                display.drawMainScreen(appState.sensor(), appState.outputs(),
                                       appState.incubationDays(), appState.uptimeSeconds(),
                                       appState.config().setpoint, turnMin, until);
            }
        }

        delay(10);
    }
}

void setup() {
    Serial.begin(115200);
    log_i("=== Incubadora ESP32 v2 ===");

    if (!configManager.begin()) {
        log_e("SPIFFS mount failed, system halting");
        while (true) delay(100);
    }
    log_i("Config loaded");

    appState = AppState();
    appState.config() = configManager.config();
    appState.setIncubationDays(configManager.incubationDays());

    sensor.begin();
    display.begin();
    encoder.begin();
    heater.begin();
    humidifier.begin();
    eggTray.begin();
    buzzer.begin();
    buzzer.setEnabled(appState.config().buzzerEnabled);
    log_i("Hardware init done");

    menuSystem.setOnValueChange(onFieldValueChange);
    menuSystem.setOnEnterEdit(onEnterEdit);
    menuSystem.setOnExitEdit(onExitEdit);
    menuSystem.setOnAction(onMenuAction);
    menuSystem.begin();

    initController();

    incubationStart = millis() - configManager.incubationElapsedS() * 1000UL;
    lastTurnTime = millis();

    esp_task_wdt_init(10, true);

    {
        const char* name = "none";
        switch (appState.controllerType()) {
            case Settings::ControllerType::PID:       name = "PID";       break;
            case Settings::ControllerType::LADRC:     name = "LADRC";    break;
            default:                                   name = "Hysteresis"; break;
        }
        log_d("Controller: %s", name);
    }

    bool staConnected = false;
    if (appState.config().ssid.length() > 0) {
        log_i("Connecting to STA %s", appState.config().ssid.c_str());
        staConnected = wifiManager.connectSTA(appState.config());
        appState.setWifiConnected(staConnected);

        if (staConnected) {
            log_i("WiFi connected, IP: %s", wifiManager.localIP().toString().c_str());
            appState.setInfoIp(wifiManager.localIP().toString());
            mqttManager.begin(appState.config());
            mqttManager.setOnMessage([](const String& topic, const String& payload) {
                log_d("MQTT msg: %s = %s", topic.c_str(), payload.c_str());
                if (topic == MqttTopics::CONFIG_REQUEST) {
                    handleMqttRequest(payload);
                } else if (topic == MqttTopics::CMD_RESTART) {
                    log_i("Restart via MQTT");
                    ESP.restart();
#if TONE_TEST_ON
                } else if (topic == MqttTopics::TONE_TEST) {
                    handleTestTone(payload);
#endif
                } else if (topic.startsWith(MqttTopics::CONFIG_PREFIX)) {
                    String key = topic.substring(strlen(MqttTopics::CONFIG_PREFIX));
                    if (key != "request" && key != "response") {
                        applyMqttSet(key, payload);
                    }
                }
            });
        } else {
            log_w("STA connection failed");
        }
    }

    if (!staConnected) {
        log_i("Starting AP mode: %s", Settings::AP_SSID);
        wifiManager.beginAP();
        appState.setApMode(true);
        appState.setInfoIp(wifiManager.softAPIP().toString());
        webServer.start(&configManager);
        g_webRunning = true;
        log_i("Web server started (AP portal)");
    }

    xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, nullptr, 1, nullptr, 0);

    log_i("Setup complete - entering loop");
}

void loop() {
    unsigned long now = millis();

    if (now - lastSensorRead >= Settings::SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        readSensor();
    }

    if (now - lastControl >= Settings::CONTROL_INTERVAL_MS) {
        lastControl = now;
        runControl();
        checkAlarms();
        checkEggTurn();
        updateIncubationDays();
    }

    if (now - lastIncubationSave >= Settings::INCUBATION_SAVE_INTERVAL_MS) {
        lastIncubationSave = now;
        if (incubationStart > 0) {
            configManager.setIncubationElapsedS((now - incubationStart) / 1000UL);
        }
    }
    configManager.saveIfDirty();

    if (mqttManager.isConnected() && now - lastMqttPublish >= Settings::MQTT_PUBLISH_INTERVAL_MS) {
        lastMqttPublish = now;
        mqttManager.publish(MqttTopics::TEMPERATURE, appState.sensor().temperature);
        mqttManager.publish(MqttTopics::HUMIDITY, appState.sensor().humidity);
        mqttManager.publish(MqttTopics::DAYS, static_cast<int32_t>(appState.incubationDays()));
    }

    esp_task_wdt_reset();
    delay(10);
}
