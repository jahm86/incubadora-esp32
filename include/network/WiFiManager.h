#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "types.h"
#include "config/settings.h"

class WiFiManager {
public:
    void beginAP();
    bool connectSTA(const SystemConfig& config);

    bool isConnected() const;
    bool isApMode() const { return m_apMode; }

    void stopAP();
    IPAddress localIP() const;
    void disconnect();
    String macAddress() const;

private:
    bool m_apMode = false;
};
