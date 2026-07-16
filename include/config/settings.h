#pragma once

#include <Arduino.h>

namespace Settings {

constexpr float DEFAULT_TEMP_OFFSET        = 0.0f;
constexpr float DEFAULT_HUM_OFFSET         = 0.0f;
constexpr float DEFAULT_SETPOINT           = 37.5f;
constexpr float DEFAULT_TEMP_ALARM_HIGH    = 38.5f;
constexpr float DEFAULT_TEMP_ALARM_LOW     = 36.5f;
constexpr float DEFAULT_HUM_SETPOINT_ON    = 55.0f;
constexpr float DEFAULT_HUM_SETPOINT_OFF   = 65.0f;
constexpr float DEFAULT_HUM_ALARM_HIGH     = 75.0f;
constexpr float DEFAULT_HUM_ALARM_LOW      = 40.0f;
constexpr uint32_t DEFAULT_TURN_INTERVAL   = 240;
constexpr uint32_t DEFAULT_TURN_DURATION    = 5;
constexpr uint32_t DEFAULT_INCUBATION_DAYS = 21;

enum class ControllerType : uint8_t {
    Hysteresis = 0,
    PID        = 1,
    LADRC      = 2
};

constexpr ControllerType DEFAULT_CONTROLLER = ControllerType::Hysteresis;

constexpr float DEFAULT_KP = 30.0f;
constexpr float DEFAULT_KI = 0.5f;
constexpr float DEFAULT_KD = 10.0f;

constexpr float DEFAULT_HYSTERESIS = 0.3f;

constexpr float DEFAULT_B0 = 25.0f;
constexpr float DEFAULT_WC = 15.0f;
constexpr float DEFAULT_WO = 60.0f;

constexpr uint32_t SENSOR_INTERVAL_MS      = 2000;
constexpr uint32_t CONTROL_INTERVAL_MS      = 1000;
constexpr uint32_t MQTT_PUBLISH_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_REFRESH_MS       = 500;

constexpr const char* AP_SSID     = "Incubadora-AP";
constexpr const char* AP_PASSWORD = "config1234";

constexpr const char* CONFIG_FILE = "/config.json";

} // namespace Settings
