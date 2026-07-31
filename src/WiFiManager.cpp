#include <Arduino.h>
#include "network/WiFiManager.h"

void WiFiManager::beginAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Settings::AP_SSID, Settings::AP_PASSWORD);
    uint32_t timeout = millis() + 5000;
    while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && millis() < timeout) {
        delay(10);
    }
    delay(1000);
    log_d("AP ready at %s", WiFi.softAPIP().toString().c_str());
    WiFi.onEvent([](WiFiEvent_t event, arduino_event_info_t info) {
        log_d("Client connected to AP: " MACSTR, MAC2STR(info.wifi_ap_staconnected.mac));
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent([](WiFiEvent_t event, arduino_event_info_t info) {
        log_d("Client disconnected from AP: " MACSTR, MAC2STR(info.wifi_ap_stadisconnected.mac));
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    m_apMode = true;
}

bool WiFiManager::connectSTA(const SystemConfig& config) {
    WiFi.mode(WIFI_STA);

    if (config.useStaticIP && config.staticIP.length() > 0) {
        IPAddress ip, gateway, netmask;
        if (ip.fromString(config.staticIP) &&
            gateway.fromString(config.staticGateway) &&
            netmask.fromString(config.staticNetmask)) {
            WiFi.config(ip, gateway, netmask);
        }
    }

    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    uint32_t timeout = millis() + 10000;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(100);
    }

    m_apMode = false;
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::stopAP() {
    WiFi.softAPdisconnect(true);
    m_apMode = false;
}

IPAddress WiFiManager::localIP() const {
    return WiFi.localIP();
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
}

String WiFiManager::macAddress() const {
    return WiFi.macAddress();
}
