#include <Arduino.h>
#include <math.h>
#include "web/WebServer.h"
#include "core/AppState.h"
#include "config/FieldRegistry.h"

WebServerManager::WebServerManager() : m_server(80) {}

void WebServerManager::begin(ConfigManager* config, AppState* appState) {
    start(config, appState);
}

void WebServerManager::start(ConfigManager* config, AppState* appState) {
    if (m_server.isRunning()) {
        return;
    }
    m_config = config;
    m_appState = appState;

    PsychicStaticFileHandler* fileHandler = m_server.serveStatic("/", SPIFFS, "/");
    fileHandler->setDefaultFile("index.html");

    m_server.addMiddleware([](PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareNext next) {
        log_d("Web: %s %s", request->method() == HTTP_GET ? "GET" : "POST", request->url().c_str());
        return next();
    });

    m_server.on("/api/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();

        m_appState->lockConfig();
        const SystemConfig& cfg = m_appState->config();
        root["ssid"]             = cfg.ssid;
        root["password"]         = cfg.password;
        root["static_ip"]        = cfg.staticIP;
        root["static_gateway"]   = cfg.staticGateway;
        root["static_netmask"]   = cfg.staticNetmask;
        root["use_static_ip"]    = cfg.useStaticIP;
        root["mqtt_server"]      = cfg.mqttServer;
        root["mqtt_port"]        = cfg.mqttPort;
        root["mqtt_user"]        = cfg.mqttUser;
        root["mqtt_password"]    = cfg.mqttPassword;
        root["mqtt_cert"]        = cfg.mqttCert;
        root["mqtt_tls"]         = cfg.mqttUseTLS;
        root["temp_offset"]      = cfg.tempOffset;
        root["hum_offset"]       = cfg.humOffset;
        root["setpoint"]         = cfg.setpoint;
        root["temp_alarm_high"]  = cfg.tempAlarmHigh;
        root["temp_alarm_low"]   = cfg.tempAlarmLow;
        root["hum_on"]           = cfg.humSetpointOn;
        root["hum_off"]          = cfg.humSetpointOff;
        root["hum_alarm_high"]   = cfg.humAlarmHigh;
        root["hum_alarm_low"]    = cfg.humAlarmLow;
        root["turn_interval"]    = cfg.turnInterval;
        root["turn_duration"]    = cfg.turnDuration;
        root["turn_display_mode"]= cfg.turnDisplayMode;
        root["buzzer_enabled"]   = cfg.buzzerEnabled;
        root["controller_type"]  = cfg.controllerType;
        root["kp"]               = cfg.kp;
        root["ki"]               = cfg.ki;
        root["kd"]               = cfg.kd;
        root["hysteresis"]       = cfg.hysteresis;
        root["b0"]               = cfg.b0;
        root["wc"]               = cfg.wc;
        root["wo"]               = cfg.wo;
        m_appState->unlockConfig();

        return res.send();
    });

    m_server.on("/api/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        auto g = [&](const char* k) -> String {
            return request->hasParam(k, true) ? request->getParam(k, true)->value() : "";
        };
        auto gv = [&](const char* k) -> float {
            return request->hasParam(k, true) ? atof(request->getParam(k, true)->value().c_str()) : NAN;
        };

        m_appState->lockConfig();
        SystemConfig& cfg = m_appState->config();
        float prevSetpoint = cfg.setpoint;
        uint8_t prevController = cfg.controllerType;
        bool prevBuzzer = cfg.buzzerEnabled;
        String prevSsid = cfg.ssid;
        String prevPass = cfg.password;
        bool prevStatic = cfg.useStaticIP;
        String prevMqttServer = cfg.mqttServer;
        int prevMqttPort = cfg.mqttPort;
        String prevMqttUser = cfg.mqttUser;
        String prevMqttPass = cfg.mqttPassword;
        bool prevMqttTls = cfg.mqttUseTLS;

        if (request->hasParam("ssid", true))           cfg.ssid            = g("ssid");
        if (request->hasParam("password", true))       cfg.password        = g("password");
        if (request->hasParam("static_ip", true))      cfg.staticIP        = g("static_ip");
        if (request->hasParam("static_gateway", true))   cfg.staticGateway   = g("static_gateway");
        if (request->hasParam("static_netmask", true))   cfg.staticNetmask   = g("static_netmask");
        if (request->hasParam("use_static_ip", true))  cfg.useStaticIP     = g("use_static_ip") == "true";
        if (request->hasParam("mqtt_server", true))    cfg.mqttServer      = g("mqtt_server");
        if (request->hasParam("mqtt_user", true))      cfg.mqttUser        = g("mqtt_user");
        if (request->hasParam("mqtt_password", true))  cfg.mqttPassword    = g("mqtt_password");
        if (request->hasParam("mqtt_cert", true))      cfg.mqttCert        = g("mqtt_cert");
        if (request->hasParam("mqtt_port", true))      cfg.mqttPort        = atoi(g("mqtt_port").c_str());
        if (request->hasParam("mqtt_tls", true))       cfg.mqttUseTLS      = g("mqtt_tls") == "true";

        // Parámetros numéricos procesados automáticamente contra FieldRegistry con validación y clamp
        for (size_t i = 0; i < FieldRegistry::count(); i++) {
            const FieldDef* def = &FieldRegistry::all()[i];
            if (def->key && def->key[0] != '\0' && request->hasParam(def->key, true)) {
                String strVal = g(def->key);
                if (strVal.length() > 0) {
                    float raw = atof(strVal.c_str());
                    if (!isnan(raw)) {
                        float clamped = FieldRegistry::clampValue(*def, raw);
                        def->set(cfg, clamped);
                    }
                }
            }
        }
        if (request->hasParam("buzzer_enabled", true)) {
            cfg.buzzerEnabled = g("buzzer_enabled") == "true";
        }

        FieldRegistry::clampCrossDependencies(cfg);

        bool controllerChanged = (fabsf(cfg.setpoint - prevSetpoint) > 1e-4f) || (cfg.controllerType != prevController);
        bool buzzerChanged = (cfg.buzzerEnabled != prevBuzzer);
        bool wifiChanged = (cfg.ssid != prevSsid) || (cfg.password != prevPass) || (cfg.useStaticIP != prevStatic);
        bool mqttChanged = (cfg.mqttServer != prevMqttServer) || (cfg.mqttPort != prevMqttPort) ||
                           (cfg.mqttUser != prevMqttUser) || (cfg.mqttPassword != prevMqttPass) ||
                           (cfg.mqttUseTLS != prevMqttTls);

        m_appState->unlockConfig();

        // Guardado diferido y side-effects ordenados en loopTask (sin tocar SPIFFS desde el hilo web)
        m_appState->requestConfigSave();
        if (controllerChanged) {
            m_appState->requestControllerInit();
        }
        if (buzzerChanged) {
            m_appState->requestBuzzerUpdate();
        }
        if (wifiChanged) {
            m_appState->requestWifiReconnect();
        }
        if (mqttChanged) {
            m_appState->requestMqttReconnect();
        }

        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();
        root["status"] = "ok";
        return res.send();
    });

    m_server.on("/api/restart", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();
        root["status"] = "ok";
        esp_err_t ret = res.send();
        m_appState->requestRestart();
        return ret;
    });

    m_server.onNotFound([](PsychicRequest* request, PsychicResponse* response) {
        response->setCode(404);
        response->setContentType("text/plain");
        return response->send("Not Found");
    });

    m_server.begin();
}

void WebServerManager::stop() {
    if (m_server.isRunning()) {
        m_server.stop();
        log_i("Web server stopped");
    }
}
