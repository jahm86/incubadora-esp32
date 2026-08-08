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
unsigned long incubationStart     = 0;

volatile bool alarmActive = false;
volatile uint32_t alarmSnoozedUntil = 0;
char alarmMessage[64] = {0};

const Tone kAlarmMelody[] = {
    {4, 10}, {2, 12}, {4, 10}, {2, 12}, {4, 10}, {2, 12}
};
const BufferTone kAlarmBuffer = {
    const_cast<Tone*>(kAlarmMelody),
    sizeof(kAlarmMelody) / sizeof(Tone),
    true
};

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
    bool cond = false;
    char msg[64] = {0};

    if (appState.sensor().valid) {
        float temp = appState.sensor().temperature;
        float hum  = appState.sensor().humidity;

        if (temp > appState.config().tempAlarmHigh) {
            cond = true;
            snprintf(msg, sizeof(msg), "Temp ALTA: %.1f C", temp);
        } else if (temp < appState.config().tempAlarmLow) {
            cond = true;
            snprintf(msg, sizeof(msg), "Temp BAJA: %.1f C", temp);
        }

        if (hum > appState.config().humAlarmHigh) {
            cond = true;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum ALTA: %.1f%%", hum);
        } else if (hum < appState.config().humAlarmLow) {
            cond = true;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum BAJA: %.1f%%", hum);
        }
    }

    bool shouldAlarm = cond && (millis() >= alarmSnoozedUntil);

    if (shouldAlarm && !alarmActive) {
        alarmActive = true;
        strncpy(alarmMessage, msg, sizeof(alarmMessage) - 1);
        alarmMessage[sizeof(alarmMessage) - 1] = '\0';
        appState.outputs().buzzerActive = true;
        buzzer.play(kAlarmBuffer);
        if (mqttManager.isConnected()) {
            mqttManager.publish(MqttTopics::ALARM, alarmMessage);
        }
    } else if (!shouldAlarm && alarmActive) {
        alarmActive = false;
        appState.outputs().buzzerActive = false;
        buzzer.stop();
    }
}

void checkEggTurn() {
    static bool turning = false;
    static unsigned long turnStart = 0;

    if (turning) {
        if (millis() - turnStart > appState.config().turnDuration * 1000UL) {
            eggTray.off();
            appState.outputs().eggTrayActive = false;
            turning = false;
        }
        return;
    }

    if (millis() - lastTurnCheck > appState.config().turnInterval * 60UL * 1000UL) {
        lastTurnCheck = millis();
        eggTray.on();
        appState.outputs().eggTrayActive = true;
        turning = true;
        turnStart = millis();
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
};

static const ConfigFieldDef* fieldDefs() {
    static ConfigFieldDef defs[static_cast<uint8_t>(MenuField::Count)];
    static bool init = false;
    if (init) {
        return defs;
    }
    init = true;

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
         1.0f, 1.0f, 60.0f, true, false};
    defs[static_cast<uint8_t>(MenuField::ControllerType)] =
        {"controller_type", "Controlador", "", [P] { return static_cast<float>(P->controllerType); },
         [P](float v) { P->controllerType = static_cast<uint8_t>(v); },
         1.0f, 0.0f, 2.0f, true, true};
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

    return defs;
}

constexpr uint8_t NODE_SYSTEM = 5;
constexpr uint8_t WEB_ITEM_IDX = 0;

bool g_webRunning = false;

void formatFieldValue(const ConfigFieldDef& def, char* buf, size_t size) {
    if (def.isEnum) {
        static const char* ctlNames[] = {"Hysteresis", "PID", "LADRC"};
        int idx = constrain(static_cast<int>(def.get()), 0, 2);
        snprintf(buf, size, "%s", ctlNames[idx]);
    } else if (def.isInt) {
        snprintf(buf, size, "%.0f %s", def.get(), def.unit);
    } else {
        snprintf(buf, size, "%.1f %s", def.get(), def.unit);
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
    v = constrain(v, def.min, def.max);
    def.set(v);

    if (f == MenuField::Setpoint || f == MenuField::ControllerType) {
        initController();
    }
}

void onEnterEdit(MenuField f) {
    const ConfigFieldDef& def = fieldDefs()[static_cast<uint8_t>(f)];
    long maxPos = lround((def.max - def.min) / def.step);
    encoder.setBoundaries(0, maxPos, false);
    long pos = lround((def.get() - def.min) / def.step);
    encoder.setValue(pos);
}

void onExitEdit(MenuField f) {
    (void)f;
    encoder.setBoundaries(0, 255, true);
    encoder.setValue(0);
    syncAndSaveConfig();
}

void updateWebLabel() {
    menuSystem.setDynamicLabel(NODE_SYSTEM, WEB_ITEM_IDX,
                               g_webRunning ? "Servidor Web: ON" : "Servidor Web: OFF");
}

void toggleWeb() {
    if (g_webRunning) {
        webServer.stop();
        g_webRunning = false;
        log_i("Web server OFF");
    } else {
        webServer.start(&configManager);
        g_webRunning = true;
        log_i("Web server ON");
    }
    updateWebLabel();
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
    } else if (action == 2) {
        toggleWeb();
    } else if (action == 3) {
        factoryReset();
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

        if (encoder.wasClicked()) {
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

        int encVal = encoder.getValue();
        if (encVal != lastEncVal) {
            int delta = encVal - lastEncVal;
            lastEncVal = encVal;
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
                formatFieldValue(def, valBuf, sizeof(valBuf));
                display.drawEditValue(def.label, valBuf);
            } else if (menuSystem.currentPage() == MenuPage::Info) {
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
            } else if (menuSystem.currentPage() == MenuPage::Confirm) {
                const char* title;
                const char* line;
                if (menuSystem.pendingAction() == 1) {
                    title = "Reset Dias";
                    line = "Reset diario?";
                } else {
                    title = "Restaurar Fab.";
                    line = "Borrar config?";
                }
                display.drawConfirm(title, line, menuSystem.confirmChoice());
            } else if (menuSystem.currentPage() == MenuPage::Menu) {
                display.drawMenu(menuSystem);
            } else {
                display.drawMainScreen(appState.sensor(), appState.outputs(),
                                       appState.incubationDays(), appState.uptimeSeconds(),
                                       appState.config().setpoint);
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
    log_i("Hardware init done");

    menuSystem.setOnValueChange(onFieldValueChange);
    menuSystem.setOnEnterEdit(onEnterEdit);
    menuSystem.setOnExitEdit(onExitEdit);
    menuSystem.setOnAction(onMenuAction);
    menuSystem.begin();

    initController();

    incubationStart = millis() - configManager.incubationElapsedS() * 1000UL;

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

    updateWebLabel();

    xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, nullptr, 1, nullptr, 0);

    log_i("Setup complete - entering loop");
}

void loop() {
    unsigned long now = millis();
    appState.tickUptime();

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
