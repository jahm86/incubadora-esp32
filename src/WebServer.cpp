#include <Arduino.h>
#include <math.h>
#include "web/WebServer.h"

WebServerManager::WebServerManager() : m_server(80) {}

void WebServerManager::begin(ConfigManager* config) {
    start(config);
}

void WebServerManager::start(ConfigManager* config) {
    if (m_server.isRunning()) {
        return;
    }
    m_config = config;

    PsychicStaticFileHandler* fileHandler = m_server.serveStatic("/", SPIFFS, "/");
    fileHandler->setDefaultFile("index.html");

    m_server.addMiddleware([](PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareNext next) {
        log_d("Web: %s %s", request->method() == HTTP_GET ? "GET" : "POST", request->url().c_str());
        return next();
    });

    m_server.on("/api/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();
        SystemConfig& cfg = m_config->config();
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
        root["controller_type"]  = cfg.controllerType;
        root["kp"]               = cfg.kp;
        root["ki"]               = cfg.ki;
        root["kd"]               = cfg.kd;
        root["hysteresis"]       = cfg.hysteresis;
        root["b0"]               = cfg.b0;
        root["wc"]               = cfg.wc;
        root["wo"]               = cfg.wo;
        return res.send();
    });

    m_server.on("/api/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        SystemConfig& cfg = m_config->config();
        auto g = [&](const char* k) -> String {
            return request->hasParam(k, true) ? request->getParam(k, true)->value() : "";
        };

        if (request->hasParam("ssid", true))         cfg.ssid            = g("ssid");
        if (request->hasParam("password", true))     cfg.password        = g("password");
        if (request->hasParam("static_ip", true))    cfg.staticIP        = g("static_ip");
        if (request->hasParam("static_gateway", true)) cfg.staticGateway = g("static_gateway");
        if (request->hasParam("static_netmask", true)) cfg.staticNetmask = g("static_netmask");
        if (request->hasParam("use_static_ip", true)) cfg.useStaticIP   = g("use_static_ip") == "true";
        if (request->hasParam("mqtt_server", true))  cfg.mqttServer      = g("mqtt_server");
        if (request->hasParam("mqtt_user", true))    cfg.mqttUser        = g("mqtt_user");
        if (request->hasParam("mqtt_password", true)) cfg.mqttPassword   = g("mqtt_password");
        if (request->hasParam("mqtt_cert", true))    cfg.mqttCert        = g("mqtt_cert");
        if (request->hasParam("mqtt_port", true))    cfg.mqttPort        = atoi(g("mqtt_port").c_str());
        if (request->hasParam("mqtt_tls", true))     cfg.mqttUseTLS      = g("mqtt_tls") == "true";

        auto gv = [&](const char* k) -> float {
            return request->hasParam(k, true) ? atof(request->getParam(k, true)->value().c_str()) : NAN;
        };
        float v;
        if (!isnan((v = gv("temp_offset"))))         cfg.tempOffset      = v;
        if (!isnan((v = gv("hum_offset"))))          cfg.humOffset       = v;
        if (!isnan((v = gv("setpoint"))))            cfg.setpoint        = v;
        if (!isnan((v = gv("temp_alarm_high"))))     cfg.tempAlarmHigh   = v;
        if (!isnan((v = gv("temp_alarm_low"))))      cfg.tempAlarmLow    = v;
        if (!isnan((v = gv("hum_on"))))              cfg.humSetpointOn   = v;
        if (!isnan((v = gv("hum_off"))))             cfg.humSetpointOff  = v;
        if (!isnan((v = gv("hum_alarm_high"))))      cfg.humAlarmHigh    = v;
        if (!isnan((v = gv("hum_alarm_low"))))       cfg.humAlarmLow     = v;
        if (!isnan((v = gv("turn_interval"))))       cfg.turnInterval    = static_cast<uint16_t>(v);
        if (!isnan((v = gv("turn_duration"))))       cfg.turnDuration    = static_cast<uint16_t>(v);
        if (!isnan((v = gv("controller_type"))))     cfg.controllerType  = static_cast<uint8_t>(v);
        if (!isnan((v = gv("kp"))))                  cfg.kp              = v;
        if (!isnan((v = gv("ki"))))                  cfg.ki              = v;
        if (!isnan((v = gv("kd"))))                  cfg.kd              = v;
        if (!isnan((v = gv("hysteresis"))))          cfg.hysteresis      = v;
        if (!isnan((v = gv("b0"))))                  cfg.b0              = v;
        if (!isnan((v = gv("wc"))))                  cfg.wc              = v;
        if (!isnan((v = gv("wo"))))                  cfg.wo              = v;

        m_config->save();
        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();
        root["status"] = "ok";
        return res.send();
    });

    m_server.on("/api/restart", HTTP_POST, [](PsychicRequest* request, PsychicResponse* response) {
        PsychicJsonResponse res = PsychicJsonResponse(response);
        JsonObject root = res.getRoot();
        root["status"] = "ok";
        esp_err_t ret = res.send();
        delay(100);
        ESP.restart();
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
