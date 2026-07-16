#pragma once

#include <Arduino.h>
#include <functional>

struct SensorData {
    float temperature = NAN;
    float humidity    = NAN;
    bool  valid       = false;
};

struct OutputState {
    bool  heaterActive     = false;
    float heaterPower      = 0.0f;
    bool  humidifierActive = false;
    bool  eggTrayActive    = false;
    bool  buzzerActive     = false;
};

struct SystemConfig {
    float tempOffset         = 0.0f;
    float humOffset          = 0.0f;
    float setpoint           = 37.5f;
    float tempAlarmHigh      = 38.5f;
    float tempAlarmLow       = 36.5f;
    float humSetpointOn      = 55.0f;
    float humSetpointOff     = 65.0f;
    float humAlarmHigh       = 75.0f;
    float humAlarmLow        = 40.0f;
    uint32_t turnInterval    = 240;
    uint32_t turnDuration    = 5;
    bool useStaticIP         = false;
    String ssid;
    String password;
    String staticIP;
    String staticGateway;
    String staticNetmask;
    String mqttServer;
    uint16_t mqttPort       = 1883;
    String mqttUser;
    String mqttPassword;
    bool mqttUseTLS         = false;
    String mqttCert;
    uint8_t controllerType  = 0;
    float kp                = 30.0f;
    float ki                = 0.5f;
    float kd                = 10.0f;
    float hysteresis        = 0.3f;
    float b0                = 25.0f;
    float wc                = 15.0f;
    float wo                = 60.0f;
};

enum class MenuPage : uint8_t {
    Main,
    Config,
    EditValue
};

using AlarmCallback = std::function<void(const String& message)>;
