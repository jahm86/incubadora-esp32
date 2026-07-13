#pragma once

namespace MqttTopics {

constexpr const char* TEMPERATURE     = "incubadora/status/temperatura";
constexpr const char* HUMIDITY        = "incubadora/status/humedad";
constexpr const char* DAYS            = "incubadora/status/dias";
constexpr const char* ALARM           = "incubadora/status/alarma";
constexpr const char* SETPOINT        = "incubadora/config/setpoint";
constexpr const char* HUM_ON          = "incubadora/config/humidificador_on";
constexpr const char* HUM_OFF        = "incubadora/config/humidificador_off";
constexpr const char* TURN_INTERVAL   = "incubadora/config/volteo_intervalo";
constexpr const char* TURN_DURATION   = "incubadora/config/volteo_duracion";
constexpr const char* CONTROLLER_TYPE = "incubadora/config/controller_type";
constexpr const char* CMD_RESTART     = "incubadora/cmd/restart";

} // namespace MqttTopics
