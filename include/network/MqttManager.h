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
    using MessageCallback = void (*)(const String&, const String&);

    MqttManager();

    void begin(const SystemConfig& config);
    void setOnMessage(MessageCallback cb);

    void loop(bool wifiConnected, const SystemConfig& config);

    bool isConnected() const { return m_connected; }
    MqttConnectionState state() const { return m_state; }
    const char* stateText() const;

    bool publish(const char* topic, const char* payload);
    bool publish(const char* topic, float value);
    bool publish(const char* topic, int32_t value);

    void disconnect();
    void reconnect(const SystemConfig& config);

private:
    PsychicMqttClient m_client;
    bool m_connected = false;
    bool m_callbacksAttached = false;
    uint32_t m_lastMqttAttempt = 0;
    MqttConnectionState m_state = MqttConnectionState::NoConfigurado;
    MessageCallback m_onMessage = nullptr;

    void startConnection(const SystemConfig& config);
    void subscribe();
};
