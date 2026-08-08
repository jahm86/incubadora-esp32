#include <Arduino.h>
#include "network/MqttManager.h"

MqttManager::MqttManager() : m_client() {}

void MqttManager::begin(const SystemConfig& config) {
    String uri;
    if (config.mqttUseTLS) {
        uri = "mqtts://";
    } else {
        uri = "mqtt://";
    }

    if (config.mqttUser.length() > 0) {
        uri += config.mqttUser;
        if (config.mqttPassword.length() > 0) {
            uri += ":" + config.mqttPassword;
        }
        uri += "@";
    }

    uri += config.mqttServer;
    uri += ":" + String(config.mqttPort);

    m_client.setServer(uri.c_str());

    if (config.mqttUseTLS && config.mqttCert.length() > 0) {
        m_client.setCACert(config.mqttCert.c_str());
    }

    m_client.onConnect([this](bool sessionPresent) {
        m_connected = true;
        subscribe();
    });

    m_client.onDisconnect([this](bool reason) {
        m_connected = false;
    });

    m_client.onMessage([this](char* topic, char* payload, int retain, int qos, bool dup) {
        if (m_onMessage) {
            m_onMessage(String(topic), String(payload));
        }
    });

    m_client.connect();
}

void MqttManager::setOnMessage(void (*cb)(const String&, const String&)) {
    m_onMessage = cb;
}

bool MqttManager::publish(const char* topic, const char* payload) {
    if (!m_connected) return false;
    return m_client.publish(topic, 0, false, payload) >= 0;
}

bool MqttManager::publish(const char* topic, float value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", value);
    return publish(topic, buf);
}

bool MqttManager::publish(const char* topic, int32_t value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)value);
    return publish(topic, buf);
}

void MqttManager::disconnect() {
    m_client.disconnect();
}

void MqttManager::subscribe() {
    String configFilter = String(MqttTopics::CONFIG_PREFIX) + "#";
    m_client.subscribe(configFilter.c_str(), 0);
    m_client.subscribe(MqttTopics::CMD_RESTART, 0);
}
