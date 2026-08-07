#pragma once

#include <Arduino.h>
#include <PsychicHttp.h>
#include <PsychicCore.h>
#include <SPIFFS.h>
#include "types.h"
#include "core/ConfigManager.h"

class WebServerManager {
public:
    WebServerManager();
    void begin(ConfigManager* config);
    void start(ConfigManager* config);
    void stop();
    bool isRunning() { return m_server.isRunning(); }

private:
    PsychicHttpServer m_server;
    ConfigManager* m_config = nullptr;
};
