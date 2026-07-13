#pragma once

#include <Arduino.h>
#include <PsychicHttp.h>
#include <PsychicCore.h>
#include <SPIFFS.h>
#include "types.h"
#include "core/ConfigManager.h"

class WebServerManager {
public:
    WebServerManager() : m_server(80) {}

    void begin(ConfigManager* config) {
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
            root["mqtt_server"]      = cfg.mqttServer;
            root["mqtt_port"]        = cfg.mqttPort;
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
            return res.send();
        });

        m_server.on("/api/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
            if (request->hasParam("ssid", true)) {
                m_config->config().ssid = request->getParam("ssid", true)->value();
            }
            if (request->hasParam("password", true)) {
                m_config->config().password = request->getParam("password", true)->value();
            }
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

private:
    PsychicHttpServer m_server;
    ConfigManager* m_config = nullptr;
};
