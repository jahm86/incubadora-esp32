#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "types.h"
#include "config/settings.h"

class AppState {
public:
    AppState() {
        m_configMutex = xSemaphoreCreateMutex();
    }

    ~AppState() {
        if (m_configMutex) {
            vSemaphoreDelete(m_configMutex);
        }
    }

    // No copiable por gestionar mutex FreeRTOS
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;

    void lockConfig() const {
        if (m_configMutex) {
            xSemaphoreTake(m_configMutex, portMAX_DELAY);
        }
    }

    void unlockConfig() const {
        if (m_configMutex) {
            xSemaphoreGive(m_configMutex);
        }
    }

    void requestConfigSave() { m_saveRequested = true; }
    bool consumeConfigSaveRequest() {
        if (m_saveRequested) {
            m_saveRequested = false;
            return true;
        }
        return false;
    }

    void requestControllerInit() { m_controllerInitRequested = true; }
    bool consumeControllerInitRequest() {
        if (m_controllerInitRequested) {
            m_controllerInitRequested = false;
            return true;
        }
        return false;
    }

    void requestBuzzerUpdate() { m_buzzerUpdateRequested = true; }
    bool consumeBuzzerUpdateRequest() {
        if (m_buzzerUpdateRequested) {
            m_buzzerUpdateRequested = false;
            return true;
        }
        return false;
    }

    void requestRestart() { m_restartRequested = true; }
    bool consumeRestartRequest() {
        if (m_restartRequested) {
            m_restartRequested = false;
            return true;
        }
        return false;
    }

    void requestWifiReconnect() { m_wifiReconnectRequested = true; }
    bool consumeWifiReconnectRequest() {
        if (m_wifiReconnectRequested) {
            m_wifiReconnectRequested = false;
            return true;
        }
        return false;
    }

    void requestMqttReconnect() { m_mqttReconnectRequested = true; }
    bool consumeMqttReconnectRequest() {
        if (m_mqttReconnectRequested) {
            m_mqttReconnectRequested = false;
            return true;
        }
        return false;
    }

    SensorData& sensor() { return m_sensor; }
    const SensorData& sensor() const { return m_sensor; }

    OutputState& outputs() { return m_outputs; }
    const OutputState& outputs() const { return m_outputs; }

    SystemConfig& config() { return m_config; }
    const SystemConfig& config() const { return m_config; }

    uint32_t incubationDays() const { return m_incubationDays; }
    void setIncubationDays(uint32_t days) { m_incubationDays = days; }

    bool isWifiConnected() const { return m_wifiConnected; }
    void setWifiConnected(bool connected) { m_wifiConnected = connected; }

    bool isMqttConnected() const { return m_mqttConnected; }
    void setMqttConnected(bool connected) { m_mqttConnected = connected; }

    bool isApMode() const { return m_apMode; }
    void setApMode(bool ap) { m_apMode = ap; }

    const String& infoIp() const { return m_infoIp; }
    void setInfoIp(const String& ip) { m_infoIp = ip; }

    uint32_t uptimeSeconds() const { return millis() / 1000; }

    Settings::ControllerType controllerType() const {
        return static_cast<Settings::ControllerType>(m_config.controllerType);
    }

private:
    mutable SemaphoreHandle_t m_configMutex = nullptr;
    volatile bool m_saveRequested           = false;
    volatile bool m_controllerInitRequested = false;
    volatile bool m_buzzerUpdateRequested   = false;
    volatile bool m_restartRequested        = false;
    volatile bool m_wifiReconnectRequested  = false;
    volatile bool m_mqttReconnectRequested  = false;

    SensorData   m_sensor;
    OutputState  m_outputs;
    SystemConfig m_config;
    uint32_t     m_incubationDays = 0;
    bool         m_wifiConnected  = false;
    bool         m_mqttConnected  = false;
    bool         m_apMode         = false;
    String       m_infoIp;
};
