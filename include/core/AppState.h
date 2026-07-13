#pragma once

#include <Arduino.h>
#include "types.h"
#include "config/settings.h"

class AppState {
public:
    AppState() = default;

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

    uint32_t uptimeSeconds() const { return m_uptime; }
    void tickUptime() { m_uptime++; }

    Settings::ControllerType controllerType() const {
        return static_cast<Settings::ControllerType>(m_config.controllerType);
    }

private:
    SensorData   m_sensor;
    OutputState  m_outputs;
    SystemConfig m_config;
    uint32_t     m_incubationDays = 0;
    bool         m_wifiConnected  = false;
    bool         m_mqttConnected  = false;
    bool         m_apMode         = false;
    uint32_t     m_uptime         = 0;
};
