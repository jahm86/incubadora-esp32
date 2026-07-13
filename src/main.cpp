#include <Arduino.h>
#include "config/pins.h"
#include "config/settings.h"
#include "core/AppState.h"
#include "core/ConfigManager.h"
#include "sensor/AM2120.h"
#include "control/IController.h"
#include "control/PIDController.h"
#include "control/ARDCController.h"
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
AM2120            sensor;
PIDController     pidController(Settings::DEFAULT_KP, Settings::DEFAULT_KI, Settings::DEFAULT_KD);
ARDCController    ardcController(Settings::DEFAULT_HYSTERESIS);
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

unsigned long lastSensorRead     = 0;
unsigned long lastControl        = 0;
unsigned long lastMqttPublish    = 0;
unsigned long lastDisplayRefresh = 0;
unsigned long lastTurnCheck      = 0;
unsigned long incubationStart    = 0;

bool alarmActive = false;
String alarmMessage;

void syncAndSaveConfig() {
    configManager.config() = appState.config();
    configManager.save();
}

void initController() {
    if (activeController) {
        activeController->reset();
    }
    if (appState.controllerType() == Settings::ControllerType::PID) {
        pidController.setTunings(appState.config().kp, appState.config().ki, appState.config().kd);
        pidController.setSetpoint(appState.config().setpoint);
        activeController = &pidController;
    } else {
        ardcController.setHysteresis(appState.config().hysteresis);
        ardcController.setSetpoint(appState.config().setpoint);
        activeController = &ardcController;
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
    float setpoint = appState.config().setpoint;

    float heaterPower = activeController->compute(temp, setpoint);
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
    alarmActive = false;
    alarmMessage = "";

    if (!appState.sensor().valid) return;

    float temp = appState.sensor().temperature;
    float hum  = appState.sensor().humidity;

    if (temp > appState.config().tempAlarmHigh) {
        alarmActive = true;
        alarmMessage = "Temp ALTA: " + String(temp) + "C";
    } else if (temp < appState.config().tempAlarmLow) {
        alarmActive = true;
        alarmMessage = "Temp BAJA: " + String(temp) + "C";
    }

    if (hum > appState.config().humAlarmHigh) {
        alarmActive = true;
        alarmMessage += " Hum ALTA: " + String(hum) + "%";
    } else if (hum < appState.config().humAlarmLow) {
        alarmActive = true;
        alarmMessage += " Hum BAJA: " + String(hum) + "%";
    }

    appState.outputs().buzzerActive = alarmActive;
    if (alarmActive) {
        buzzer.alarmPattern();
        if (mqttManager.isConnected()) {
            mqttManager.publish(MqttTopics::ALARM, alarmMessage.c_str());
        }
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

void onEncoderValueChange(int delta) {
    if (menuSystem.currentPage() == MenuPage::Main) return;

    float* target = nullptr;
    float step = 1.0f;

    uint8_t idx = menuSystem.selectedItem();
    switch (idx) {
        case 0:  target = &appState.config().tempOffset;     step = 0.1f; break;
        case 1:  target = &appState.config().humOffset;      step = 0.1f; break;
        case 2:  target = &appState.config().setpoint;       step = 0.1f; break;
        case 3:  target = &appState.config().tempAlarmHigh;  step = 0.1f; break;
        case 4:  target = &appState.config().tempAlarmLow;   step = 0.1f; break;
        case 5:  target = &appState.config().humSetpointOn;  step = 0.5f; break;
        case 6:  target = &appState.config().humSetpointOff; step = 0.5f; break;
        case 7:  target = &appState.config().humAlarmHigh;   step = 0.5f; break;
        case 8:  target = &appState.config().humAlarmLow;    step = 0.5f; break;
        case 9:  // Intervalo volteo
            {
                int val = appState.config().turnInterval + delta;
                val = constrain(val, 1, 1440);
                appState.config().turnInterval = val;
                syncAndSaveConfig();
            }
            break;
        case 10: // Duracion volteo
            {
                int val = appState.config().turnDuration + delta;
                val = constrain(val, 1, 60);
                appState.config().turnDuration = val;
                syncAndSaveConfig();
            }
            break;
        case 11: // Tipo controlador
            {
                int val = appState.config().controllerType + (delta > 0 ? 1 : -1);
                val = constrain(val, 0, 1);
                appState.config().controllerType = val;
                initController();
                syncAndSaveConfig();
            }
            break;
        case 12: target = &appState.config().kp; step = 1.0f; break;
        case 13: target = &appState.config().ki; step = 0.1f; break;
        case 14: target = &appState.config().kd; step = 1.0f; break;
        case 15: target = &appState.config().hysteresis; step = 0.1f; break;
        case 16: // Reset dias
            configManager.setIncubationDays(0);
            appState.setIncubationDays(0);
            incubationStart = millis();
            break;
    }

    if (target) {
        *target += delta * step;
        if (idx == 2) {
            initController();
        }
        syncAndSaveConfig();
    }
}

void onEnterEdit(uint8_t itemIndex) {
    (void)itemIndex;
    encoder.setBoundaries(0, 100, false);
    encoder.setValue(0);
}

void onExitEdit() {
    syncAndSaveConfig();
}

void setup() {
    Serial.begin(115200);
    log_i("=== Incubadora ESP32 v1 ===");
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

    menuSystem.setOnValueChange(onEncoderValueChange);
    menuSystem.setOnEnterEdit(onEnterEdit);
    menuSystem.setOnExitEdit(onExitEdit);

    initController();
    incubationStart = millis();
    log_d("Controller: %s", activeController ? (appState.controllerType() == Settings::ControllerType::PID ? "PID" : "ARDC") : "none");

    if (appState.config().ssid.length() > 0) {
        log_i("Connecting to STA %s", appState.config().ssid.c_str());
        wifiManager.connectSTA(appState.config());
        appState.setWifiConnected(wifiManager.isConnected());

        if (wifiManager.isConnected()) {
            log_i("WiFi connected, IP: %s", wifiManager.localIP().toString().c_str());
            mqttManager.begin(appState.config());
            mqttManager.setOnMessage([](const String& topic, const String& payload) {
                log_d("MQTT msg: %s = %s", topic.c_str(), payload.c_str());
                if (topic == MqttTopics::SETPOINT) {
                    appState.config().setpoint = payload.toFloat();
                    initController();
                    syncAndSaveConfig();
                } else if (topic == MqttTopics::HUM_ON) {
                    appState.config().humSetpointOn = payload.toFloat();
                    syncAndSaveConfig();
                } else if (topic == MqttTopics::HUM_OFF) {
                    appState.config().humSetpointOff = payload.toFloat();
                    syncAndSaveConfig();
                } else if (topic == MqttTopics::CMD_RESTART) {
                    log_i("Restart via MQTT");
                    ESP.restart();
                }
            });
        } else {
            log_w("STA connection failed");
        }
    }

    if (!wifiManager.isConnected()) {
        log_i("Starting AP mode: %s", Settings::AP_SSID);
        wifiManager.beginAP();
        appState.setApMode(true);
        webServer.begin(&configManager);
        log_i("Web server started");
    }

    log_i("Setup complete — entering loop");
}

void loop() {
    unsigned long now = millis();
    appState.tickUptime();

    encoder.loop();

    if (encoder.wasClicked()) {
        if (menuSystem.currentPage() == MenuPage::EditValue) {
            menuSystem.cancel();
        } else {
            menuSystem.confirm();
        }
    }

    int encVal = encoder.getValue();
    static int lastEncVal = 0;
    if (encVal != lastEncVal) {
        int delta = encVal - lastEncVal;
        menuSystem.navigate(delta);
        lastEncVal = encVal;
    }

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
        configManager.saveIfDirty();
    }

    if (mqttManager.isConnected() && now - lastMqttPublish >= Settings::MQTT_PUBLISH_INTERVAL_MS) {
        lastMqttPublish = now;
        mqttManager.publish(MqttTopics::TEMPERATURE, appState.sensor().temperature);
        mqttManager.publish(MqttTopics::HUMIDITY, appState.sensor().humidity);
        mqttManager.publish(MqttTopics::DAYS, static_cast<int32_t>(appState.incubationDays()));
    }

    if (now - lastDisplayRefresh >= Settings::DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;

        if (alarmActive) {
            display.drawAlarm(alarmMessage.c_str());
            if (encoder.wasClicked()) {
                alarmActive = false;
            }
        } else if (menuSystem.currentPage() == MenuPage::EditValue) {
            uint8_t idx = menuSystem.selectedItem();
            float val = 0;
            const char* unit = "";
            const char* label = "";
            switch (idx) {
                case 0:  val = appState.config().tempOffset;     unit = "C"; label = "Temp Offset"; break;
                case 1:  val = appState.config().humOffset;      unit = "%"; label = "Hum Offset"; break;
                case 2:  val = appState.config().setpoint;       unit = "C"; label = "Setpoint"; break;
                case 3:  val = appState.config().tempAlarmHigh;  unit = "C"; label = "Alarma T+"; break;
                case 4:  val = appState.config().tempAlarmLow;   unit = "C"; label = "Alarma T-"; break;
                case 5:  val = appState.config().humSetpointOn;  unit = "%"; label = "Hum On"; break;
                case 6:  val = appState.config().humSetpointOff; unit = "%"; label = "Hum Off"; break;
                case 7:  val = appState.config().humAlarmHigh;   unit = "%"; label = "Alarma H+"; break;
                case 8:  val = appState.config().humAlarmLow;    unit = "%"; label = "Alarma H-"; break;
                case 9:  val = appState.config().turnInterval;   unit = "m"; label = "Int. Volteo"; break;
                case 10: val = appState.config().turnDuration;   unit = "s"; label = "Dur. Volteo"; break;
                case 11: val = appState.config().controllerType; unit = "";  label = "Controlador"; break;
                case 12: val = appState.config().kp;             unit = "";  label = "Kp"; break;
                case 13: val = appState.config().ki;             unit = "";  label = "Ki"; break;
                case 14: val = appState.config().kd;             unit = "";  label = "Kd"; break;
                case 15: val = appState.config().hysteresis;     unit = "C"; label = "Histeresis"; break;
                default: break;
            }
            display.drawEditValue(label, val, unit);
        } else if (menuSystem.currentPage() == MenuPage::Config) {
            display.drawMenu("Configuracion", MenuSystem::CONFIG_ITEMS,
                             MenuSystem::CONFIG_ITEM_COUNT, menuSystem.selectedItem());
        } else {
            display.drawMainScreen(appState.sensor(), appState.outputs(),
                                   appState.incubationDays(), appState.uptimeSeconds(),
                                   appState.config().setpoint);
        }
    }

    delay(10);
}
