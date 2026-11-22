#include <Arduino.h>
#include "network/WiFiManager.h"

WiFiManager::WiFiManager() = default;

void WiFiManager::configureStaticIP(const SystemConfig& config) {
    if (config.useStaticIP && config.staticIP.length() > 0) {
        IPAddress ip, gateway, netmask;
        if (ip.fromString(config.staticIP) &&
            gateway.fromString(config.staticGateway) &&
            netmask.fromString(config.staticNetmask)) {
            WiFi.config(ip, gateway, netmask);
            log_i("WiFi IP estatica configurada: %s", config.staticIP.c_str());
        }
    }
}

void WiFiManager::beginAP() {
    m_apMode = true;
    WiFi.mode(m_staConfigured ? WIFI_AP_STA : WIFI_AP);
    WiFi.softAP(Settings::AP_SSID, Settings::AP_PASSWORD);

    uint32_t timeout = millis() + 5000;
    while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && millis() < timeout) {
        delay(10);
    }
    log_i("AP listo en %s (SSID: %s)", WiFi.softAPIP().toString().c_str(), Settings::AP_SSID);

    WiFi.onEvent([](WiFiEvent_t event, arduino_event_info_t info) {
        log_d("Cliente conectado a AP: " MACSTR, MAC2STR(info.wifi_ap_staconnected.mac));
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, arduino_event_info_t info) {
        log_d("Cliente desconectado de AP: " MACSTR, MAC2STR(info.wifi_ap_stadisconnected.mac));
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void WiFiManager::stopAP() {
    WiFi.softAPdisconnect(true);
    m_apMode = false;
    WiFi.mode(m_staConfigured ? WIFI_STA : WIFI_OFF);
    log_i("AP detenido");
}

bool WiFiManager::connectSTA(const SystemConfig& config, uint32_t timeoutMs) {
    m_staConfigured = (config.ssid.length() > 0);
    if (!m_staConfigured) {
        return false;
    }

    WiFi.mode(m_apMode ? WIFI_AP_STA : WIFI_STA);
    configureStaticIP(config);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    uint32_t timeout = millis() + timeoutMs;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(100);
    }

    bool connected = (WiFi.status() == WL_CONNECTED);
    m_lastConnected = connected;
    if (connected) {
        log_i("WiFi STA conectado: %s (IP: %s)", config.ssid.c_str(), WiFi.localIP().toString().c_str());
    } else {
        log_w("Fallo conexion WiFi STA a %s dentro del timeout", config.ssid.c_str());
    }
    return connected;
}

void WiFiManager::startSTA(const SystemConfig& config) {
    m_staConfigured = (config.ssid.length() > 0);
    if (!m_staConfigured) {
        WiFi.disconnect(true);
        m_lastConnected = false;
        return;
    }

    WiFi.mode(m_apMode ? WIFI_AP_STA : WIFI_STA);
    configureStaticIP(config);
    WiFi.disconnect(false);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    m_lastReconnectAttempt = millis();
    m_reconnectIntervalMs = 10000;
    log_i("Iniciando conexion asincrona STA a: %s", config.ssid.c_str());
}

void WiFiManager::loop(const SystemConfig& config) {
    if (config.ssid.length() == 0) {
        return;
    }

    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected && !m_lastConnected) {
        m_lastConnected = true;
        m_reconnectIntervalMs = 10000;
        log_i("WiFi STA enlace recuperado/establecido. IP: %s", WiFi.localIP().toString().c_str());
        if (m_onConnChange) {
            m_onConnChange(true, WiFi.localIP().toString());
        }
    } else if (!connected && m_lastConnected) {
        m_lastConnected = false;
        m_lastReconnectAttempt = millis();
        log_w("WiFi STA enlace perdido");
        if (m_onConnChange) {
            m_onConnChange(false, "");
        }
    } else if (!connected) {
        uint32_t now = millis();
        if (now - m_lastReconnectAttempt >= m_reconnectIntervalMs) {
            m_lastReconnectAttempt = now;
            log_d("Reintentando conexion WiFi STA a %s (intervalo %u ms)...",
                  config.ssid.c_str(), (unsigned)m_reconnectIntervalMs);
            WiFi.begin(config.ssid.c_str(), config.password.c_str());
            if (m_reconnectIntervalMs < 30000) {
                m_reconnectIntervalMs += 5000;
            }
        }
    }
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiManager::localIP() const {
    return WiFi.localIP();
}

IPAddress WiFiManager::softAPIP() const {
    return WiFi.softAPIP();
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    m_lastConnected = false;
}

String WiFiManager::macAddress() const {
    return WiFi.macAddress();
}
