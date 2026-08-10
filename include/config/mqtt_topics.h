#pragma once

namespace MqttTopics {

// Datos a transmitir en tiempo real
constexpr const char* TEMPERATURE     = "incubadora/status/temperatura";
constexpr const char* HUMIDITY        = "incubadora/status/humedad";
constexpr const char* DAYS            = "incubadora/status/dias";
constexpr const char* ALARM           = "incubadora/status/alarma";

// Parámetros de configuración para cambiar desde un cliente remoto MQTT (se pone el valor numérico en el mensaje)
constexpr const char* CONFIG_PREFIX    = "incubadora/config/";
constexpr const char* SETPOINT         = "incubadora/config/setpoint";
constexpr const char* HUM_ON           = "incubadora/config/hum_on";
constexpr const char* HUM_OFF          = "incubadora/config/hum_off";
constexpr const char* TEMP_OFFSET      = "incubadora/config/temp_offset";
constexpr const char* HUM_OFFSET       = "incubadora/config/hum_offset";
constexpr const char* TEMP_ALARM_HIGH  = "incubadora/config/temp_alarm_high";
constexpr const char* TEMP_ALARM_LOW   = "incubadora/config/temp_alarm_low";
constexpr const char* HUM_ALARM_HIGH   = "incubadora/config/hum_alarm_high";
constexpr const char* HUM_ALARM_LOW    = "incubadora/config/hum_alarm_low";
constexpr const char* TURN_INTERVAL    = "incubadora/config/turn_interval";
constexpr const char* TURN_DURATION    = "incubadora/config/turn_duration";
constexpr const char* CONTROLLER_TYPE  = "incubadora/config/controller_type";
constexpr const char* KP               = "incubadora/config/kp";
constexpr const char* KI               = "incubadora/config/ki";
constexpr const char* KD               = "incubadora/config/kd";
constexpr const char* HYSTERESIS       = "incubadora/config/hysteresis";
constexpr const char* B0_COEFF         = "incubadora/config/b0";
constexpr const char* WC               = "incubadora/config/wc";
constexpr const char* WO               = "incubadora/config/wo";

// Solicitud de valores de los parámetros desde una aplicación remota. Se publica el nombre del parámetro
// y el equipo responde en response con [nombre_parámetro]:[valor]
constexpr const char* CONFIG_REQUEST   = "incubadora/config/request";
constexpr const char* CONFIG_RESPONSE  = "incubadora/config/response";

// Reinicia el controlador
constexpr const char* CMD_RESTART      = "incubadora/cmd/restart";

// Cuando se habilita (flag TONE_TEST_ON), permite mandar a sonar una melodía mientras el buzzer esté activo.
// Detalles del formato del mensaje en el README.md
#if TONE_TEST_ON
constexpr const char* TONE_TEST         = "incubadora/test/tone";
#endif

} // namespace MqttTopics