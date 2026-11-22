#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <functional>
#include "types.h"
#include "config/settings.h"

class WiFiManager {
public:
    using ConnectionCallback = std::function<void(bool connected, const String& ip)>;

    WiFiManager();

    void beginAP();
    void stopAP();

    bool connectSTA(const SystemConfig& config, uint32_t timeoutMs = 5000);
    void startSTA(const SystemConfig& config);
    void disconnect();

    void loop(const SystemConfig& config);

    bool isConnected() const;
    bool isApMode() const { return m_apMode; }

    IPAddress localIP() const;
    IPAddress softAPIP() const;
    String macAddress() const;

    void setOnConnectionChange(ConnectionCallback cb) { m_onConnChange = cb; }

private:
    bool m_apMode = false;
    bool m_staConfigured = false;
    bool m_lastConnected = false;
    uint32_t m_lastReconnectAttempt = 0;
    uint32_t m_reconnectIntervalMs = 10000;
    ConnectionCallback m_onConnChange = nullptr;

    void configureStaticIP(const SystemConfig& config);
};
