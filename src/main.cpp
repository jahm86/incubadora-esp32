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
#include "config/FieldRegistry.h"
#include "input/RotaryEncoder.h"
#include "network/WiFiManager.h"
#include "network/MqttManager.h"
#include "web/WebServer.h"
#include "output/Buzzer.h"
#include "output/Heater.h"
#include "output/Humidifier.h"
#include "output/EggTray.h"
#include "alarm/AlarmManager.h"
#include "core/IncubationTracker.h"

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
AlarmManager      alarmManager;
IncubationTracker incubationTracker;

unsigned long lastSensorRead      = 0;
unsigned long lastControl         = 0;
unsigned long lastMqttPublish     = 0;
unsigned long lastIncubationSave  = 0;
unsigned long lastTurnCheck       = 0;
unsigned long lastTurnTime        = 0;

bool g_turning = false;
unsigned long g_turnStart = 0;

const Tone kUiClick[] = {{4, 2}, {4, 1}};
const Tone kUiTick[]  = {{0, 4}};

void syncAndSaveConfig() {
    appState.lockConfig();
    configManager.config() = appState.config();
    appState.unlockConfig();
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
    appState.outputs().heaterPower = heater.getPower();
    appState.outputs().heaterActive = heater.isActive();

    float hum = appState.sensor().humidity;
    if (hum < appState.config().humSetpointOn) {
        humidifier.on();
    } else if (hum > appState.config().humSetpointOff) {
        humidifier.off();
    }
    appState.outputs().humidifierActive = humidifier.isActive();
}



void startTurn() {
    eggTray.on();
    appState.outputs().eggTrayActive = eggTray.isActive();
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
            appState.outputs().eggTrayActive = eggTray.isActive();
            g_turning = false;
        }
        return;
    }

    if (millis() - lastTurnCheck > appState.config().turnInterval * 60UL * 1000UL) {
        startTurn();
    }
}



bool g_webRunning = false;

static float g_editOriginal = 0.0f;
static float g_editCurrent = 0.0f;

void onFieldValueChange(int delta) {
    (void)delta;
    MenuField f = menuSystem.editField();
    const FieldDef* def = FieldRegistry::get(f);
    if (!def) return;

    long pos = encoder.getValue();
    float v = def->min + static_cast<float>(pos) * def->step;
    g_editCurrent = FieldRegistry::clampValue(*def, v);
}

void onEnterEdit(MenuField f) {
    const FieldDef* def = FieldRegistry::get(f);
    if (!def) return;

    if (f == MenuField::WebEnabled) {
        g_editOriginal = g_webRunning ? 1.0f : 0.0f;
    } else {
        appState.lockConfig();
        g_editOriginal = def->get(appState.config());
        appState.unlockConfig();
    }
    long maxPos = lround((def->max - def->min) / def->step);
    encoder.setBoundaries(0, maxPos, false);
    long pos = lround((g_editOriginal - def->min) / def->step);
    encoder.setValue(pos);
    g_editCurrent = g_editOriginal;
}

void onExitEdit(MenuField f) {
    encoder.setBoundaries(0, 255, true);
    encoder.setValue(0);

    if (fabsf(g_editCurrent - g_editOriginal) > 1e-4f) {
        const FieldDef* def = FieldRegistry::get(f);
        if (!def) return;

        if (f == MenuField::WebEnabled) {
            bool on = g_editCurrent >= 0.5f;
            if (on != g_webRunning) {
                g_webRunning = on;
                if (on) {
                    webServer.start(&configManager, &appState);
                    log_i("Web server ON");
                } else {
                    webServer.stop();
                    log_i("Web server OFF");
                }
            }
        } else {
            appState.lockConfig();
            def->set(appState.config(), g_editCurrent);
            FieldRegistry::clampCrossDependencies(appState.config());
            appState.unlockConfig();

            if (f == MenuField::Setpoint || f == MenuField::ControllerType) {
                initController();
            } else if (f == MenuField::BuzzerEnabled) {
                buzzer.setEnabled(appState.config().buzzerEnabled);
            }
            syncAndSaveConfig();
        }
        log_i("Edit commit: %s = %g", def->label, (double)g_editCurrent);
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

void onMenuAction(MenuAction action) {
    switch (action) {
        case MenuAction::ResetDays:
            incubationTracker.reset();
            configManager.setIncubationDays(0);
            configManager.setIncubationElapsedS(0);
            appState.setIncubationDays(0);
            syncAndSaveConfig();
            log_i("Incubation days reset");
            break;
        case MenuAction::FactoryReset:
            factoryReset();
            break;
        case MenuAction::TurnNow:
            startTurn();
            break;
        case MenuAction::Restart:
            log_i("Restart requested via menu");
            delay(100);
            ESP.restart();
            break;
        default:
            break;
    }
}

void applyMqttSet(const String& key, const String& payload) {
    const FieldDef* def = FieldRegistry::findByKey(key);
    if (!def) {
        log_d("MQTT: unknown config param %s", key.c_str());
        return;
    }
    float v = payload.toFloat();
    if (isnan(v)) return;
    v = FieldRegistry::clampValue(*def, v);
    appState.lockConfig();
    def->set(appState.config(), v);
    FieldRegistry::clampCrossDependencies(appState.config());
    appState.unlockConfig();

    if (key == "setpoint" || key == "controller_type") {
        initController();
    } else if (key == "buzzer_enabled") {
        buzzer.setEnabled(appState.config().buzzerEnabled);
    }
    log_i("MQTT set: %s = %g", key.c_str(), (double)v);
    syncAndSaveConfig();
}

void handleMqttRequest(const String& payload) {
    String name = payload;
    name.trim();
    const FieldDef* def = FieldRegistry::findByKey(name);
    char resp[48];
    if (!def) {
        snprintf(resp, sizeof(resp), "error(desconocido):%s", name.c_str());
        mqttManager.publish(MqttTopics::CONFIG_RESPONSE, resp);
        return;
    }
    appState.lockConfig();
    float val = def->get(appState.config());
    appState.unlockConfig();

    if (def->isInt || def->isEnum) {
        snprintf(resp, sizeof(resp), "%s:%d", name.c_str(), static_cast<int>(roundf(val)));
    } else {
        snprintf(resp, sizeof(resp), "%s:%.1f", name.c_str(), val);
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

        bool needDisplayUpdate = false;

        if (menuSystem.isEditing() != wasEditing) {
            wasEditing = menuSystem.isEditing();
            lastEncVal = encoder.getValue();
            needDisplayUpdate = true;
        }

        int btn = encoder.pollButton();
        if (btn == 1) {
            if (!alarmManager.isActive()) {
                buzzer.playNonBlocking(kUiClick, sizeof(kUiClick) / sizeof(Tone), false);
            }
        } else if (btn == -1) {
            needDisplayUpdate = true;
            if (alarmManager.isActive()) {
                alarmManager.snooze(Settings::ALARM_SNOOZE_MS);
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
            needDisplayUpdate = true;
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
            if (!alarmManager.isActive()) {
                buzzer.playNonBlocking(kUiTick, sizeof(kUiTick) / sizeof(Tone), false);
            }
            menuSystem.navigate(delta);
            needDisplayUpdate = true;
        }

        uint32_t now = millis();
        if (needDisplayUpdate || (now - lastDisplay >= Settings::DISPLAY_REFRESH_MS)) {
            lastDisplay = now;

            if (alarmManager.isActive()) {
                display.drawAlarm(alarmManager.message());
            } else if (menuSystem.isEditing()) {
                const FieldDef* def = FieldRegistry::get(menuSystem.editField());
                if (def) {
                    char valBuf[24];
                    FieldRegistry::formatValue(*def, g_editCurrent, valBuf, sizeof(valBuf));
                    display.drawEditValue(def->label, valBuf);
                }
            } else if (menuSystem.currentPage() == MenuPage::Info) {
                if (menuSystem.infoId() == InfoId::Mqtt) {
                    char line1[24], line2[24];
                    snprintf(line1, sizeof(line1), "%s", mqttManager.stateText());
                    appState.lockConfig();
                    String broker = appState.config().mqttServer;
                    appState.unlockConfig();
                    if (broker.length() > 0) {
                        snprintf(line2, sizeof(line2), "Broker: %s", broker.c_str());
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
                if (menuSystem.pendingAction() == MenuAction::ResetDays) {
                    title = "Reset Dias";
                    line = "Reset diario?";
                } else if (menuSystem.pendingAction() == MenuAction::TurnNow) {
                    title = "Volteo";
                    line = "Voltear ahora?";
                } else if (menuSystem.pendingAction() == MenuAction::Restart) {
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

    incubationTracker.begin(configManager.incubationElapsedS());
    alarmManager.begin(&buzzer, &appState.outputs());
    alarmManager.setPublishHook([](const char* topic, const char* payload) {
        if (mqttManager.isConnected()) {
            mqttManager.publish(topic, payload);
        }
    });
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

    wifiManager.setOnConnectionChange([](bool connected, const String& ip) {
        appState.setWifiConnected(connected);
        if (connected) {
            appState.setInfoIp(ip);
            appState.lockConfig();
            bool hasMqtt = (appState.config().mqttServer.length() > 0);
            appState.unlockConfig();
            if (hasMqtt) {
                mqttManager.reconnect(appState.config());
            }
        } else {
            if (appState.isApMode()) {
                appState.setInfoIp(wifiManager.softAPIP().toString());
            } else {
                appState.setInfoIp("");
            }
        }
    });

    mqttManager.setOnMessage([](const String& topic, const String& payload) {
        log_d("MQTT msg: %s = %s", topic.c_str(), payload.c_str());
        if (topic == MqttTopics::CONFIG_REQUEST) {
            handleMqttRequest(payload);
        } else if (topic == MqttTopics::CMD_RESTART) {
            log_i("Restart via MQTT requested");
            appState.requestRestart();
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

    bool staConnected = false;
    if (appState.config().ssid.length() > 0) {
        log_i("Connecting to STA %s", appState.config().ssid.c_str());
        staConnected = wifiManager.connectSTA(appState.config(), 5000);
        appState.setWifiConnected(staConnected);

        if (staConnected) {
            log_i("WiFi connected, IP: %s", wifiManager.localIP().toString().c_str());
            appState.setInfoIp(wifiManager.localIP().toString());
            mqttManager.begin(appState.config());
        } else {
            log_w("STA initial connection timeout, fallback to AP");
        }
    }

    if (!staConnected) {
        log_i("Starting AP mode: %s", Settings::AP_SSID);
        wifiManager.beginAP();
        appState.setApMode(true);
        appState.setInfoIp(wifiManager.softAPIP().toString());
        webServer.start(&configManager, &appState);
        g_webRunning = true;
        log_i("Web server started (AP portal)");
    }

    xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, nullptr, 1, nullptr, 0);

    log_i("Setup complete - entering loop");
}

void loop() {
    if (appState.consumeConfigSaveRequest()) {
        syncAndSaveConfig();
    }
    if (appState.consumeControllerInitRequest()) {
        initController();
    }
    if (appState.consumeBuzzerUpdateRequest()) {
        buzzer.setEnabled(appState.config().buzzerEnabled);
    }
    if (appState.consumeWifiReconnectRequest()) {
        log_i("Nueva config WiFi recibida, iniciando reconexion STA");
        wifiManager.startSTA(appState.config());
    }
    if (appState.consumeMqttReconnectRequest()) {
        log_i("Nueva config MQTT recibida, reconectando");
        mqttManager.reconnect(appState.config());
    }
    if (appState.consumeRestartRequest()) {
        log_i("Safe restart requested, saving config and rebooting");
        syncAndSaveConfig();
        delay(200);
        ESP.restart();
    }

    // Gestion periodica no bloqueante de red
    wifiManager.loop(appState.config());
    mqttManager.loop(wifiManager.isConnected(), appState.config());

    unsigned long now = millis();

    if (now - lastSensorRead >= Settings::SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        readSensor();
    }

    if (now - lastControl >= Settings::CONTROL_INTERVAL_MS) {
        lastControl = now;
        runControl();
        alarmManager.evaluate(appState.sensor(), appState.config());
        checkEggTurn();

        uint32_t currentDays = incubationTracker.days();
        if (currentDays != appState.incubationDays()) {
            appState.setIncubationDays(currentDays);
            configManager.setIncubationDays(currentDays);
        }
    }

    if (now - lastIncubationSave >= Settings::INCUBATION_SAVE_INTERVAL_MS) {
        lastIncubationSave = now;
        configManager.setIncubationElapsedS(incubationTracker.elapsedSeconds());
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
