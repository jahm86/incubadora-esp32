#pragma once

#include <Arduino.h>
#include <PsychicMqttClient.h>
#include "types.h"
#include "config/mqtt_topics.h"

enum class MqttConnectionState : uint8_t {
    NoConfigurado,
    Conectando,
    Conectado,
    AuthFallido,
    CertRechazado,
    ServidorNoEncontrado,
    TlsFallido
};

class MqttManager {
public:
    MqttManager();

    void begin(const SystemConfig& config);
    void setOnMessage(void (*cb)(const String&, const String&));

    bool isConnected() const { return m_connected; }

    MqttConnectionState state() const { return m_state; }
    const char* stateText() const;

    bool publish(const char* topic, const char* payload);
    bool publish(const char* topic, float value);
    bool publish(const char* topic, int32_t value);

    void disconnect();

private:
    PsychicMqttClient m_client;
    bool m_connected = false;
    MqttConnectionState m_state = MqttConnectionState::NoConfigurado;
    void (*m_onMessage)(const String&, const String&) = nullptr;

    void subscribe();
};
