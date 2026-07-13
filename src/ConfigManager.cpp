#include <Arduino.h>
#include "core/ConfigManager.h"

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(false)) {
        log_w("SPIFFS mount failed, formatting...");
        if (!SPIFFS.begin(true)) {
            log_e("SPIFFS format failed");
            return false;
        }
    }
    return load();
}

bool ConfigManager::load() {
    if (!SPIFFS.exists(Settings::CONFIG_FILE)) {
        log_i("No config file, creating defaults");
        save();
        return true;
    }

    JsonDocument doc;

    if (!readFromFile(Settings::CONFIG_FILE, doc)) {
        if (SPIFFS.exists(BACKUP_FILE)) {
            log_w("Config corrupt, trying backup");
            if (readFromFile(BACKUP_FILE, doc)) {
                log_i("Backup loaded successfully");
            } else {
                log_e("Backup also corrupt, using defaults");
                resetToDefaults();
                return true;
            }
        } else {
            log_w("Config corrupt, no backup, using defaults");
            resetToDefaults();
            return true;
        }
    }

    uint32_t fileVersion = doc["version"] | 0;
    migrateConfig(doc, fileVersion);

    m_config.ssid           = doc["ssid"] | "";
    m_config.password       = doc["password"] | "";
    m_config.staticIP       = doc["static_ip"] | "";
    m_config.staticGateway  = doc["static_gateway"] | "";
    m_config.staticNetmask  = doc["static_netmask"] | "";
    m_config.useStaticIP    = doc["use_static_ip"] | false;
    m_config.mqttServer     = doc["mqtt_server"] | "";
    m_config.mqttPort       = doc["mqtt_port"] | 1883;
    m_config.mqttUser       = doc["mqtt_user"] | "";
    m_config.mqttPassword   = doc["mqtt_password"] | "";
    m_config.mqttUseTLS     = doc["mqtt_tls"] | false;
    m_config.mqttCert       = doc["mqtt_cert"] | "";
    m_config.tempOffset     = doc["temp_offset"] | Settings::DEFAULT_TEMP_OFFSET;
    m_config.humOffset      = doc["hum_offset"] | Settings::DEFAULT_HUM_OFFSET;
    m_config.setpoint       = doc["setpoint"] | Settings::DEFAULT_SETPOINT;
    m_config.tempAlarmHigh  = doc["temp_alarm_high"] | Settings::DEFAULT_TEMP_ALARM_HIGH;
    m_config.tempAlarmLow   = doc["temp_alarm_low"] | Settings::DEFAULT_TEMP_ALARM_LOW;
    m_config.humSetpointOn  = doc["hum_on"] | Settings::DEFAULT_HUM_SETPOINT_ON;
    m_config.humSetpointOff = doc["hum_off"] | Settings::DEFAULT_HUM_SETPOINT_OFF;
    m_config.humAlarmHigh   = doc["hum_alarm_high"] | Settings::DEFAULT_HUM_ALARM_HIGH;
    m_config.humAlarmLow    = doc["hum_alarm_low"] | Settings::DEFAULT_HUM_ALARM_LOW;
    m_config.turnInterval   = doc["turn_interval"] | Settings::DEFAULT_TURN_INTERVAL;
    m_config.turnDuration   = doc["turn_duration"] | Settings::DEFAULT_TURN_DURATION;
    m_config.controllerType = doc["controller_type"] | static_cast<uint8_t>(Settings::DEFAULT_CONTROLLER);
    m_config.kp             = doc["kp"] | Settings::DEFAULT_KP;
    m_config.ki             = doc["ki"] | Settings::DEFAULT_KI;
    m_config.kd             = doc["kd"] | Settings::DEFAULT_KD;
    m_config.hysteresis     = doc["hysteresis"] | Settings::DEFAULT_HYSTERESIS;
    m_incubationDays        = doc["incubation_days"] | 0;

    validateAndClamp();
    m_dirty = false;

    log_i("Loaded config v%u | WiFi=%s MQTT=%s:%u | Setpoint=%.1fC",
          fileVersion,
          m_config.ssid.c_str(),
          m_config.mqttServer.c_str(),
          m_config.mqttPort,
          m_config.setpoint);

    return true;
}

bool ConfigManager::save() {
    if (!writeToFile(TEMP_FILE)) {
        return false;
    }

    if (SPIFFS.exists(Settings::CONFIG_FILE)) {
        if (SPIFFS.exists(BACKUP_FILE)) {
            SPIFFS.remove(BACKUP_FILE);
        }
        SPIFFS.rename(Settings::CONFIG_FILE, BACKUP_FILE);
    }

    if (!SPIFFS.rename(TEMP_FILE, Settings::CONFIG_FILE)) {
        log_e("Atomic rename failed, restoring backup");
        SPIFFS.rename(BACKUP_FILE, Settings::CONFIG_FILE);
        return false;
    }

    m_dirty = false;
    log_i("Config saved");
    return true;
}

void ConfigManager::resetToDefaults() {
    m_config = SystemConfig();
    m_config.tempOffset     = Settings::DEFAULT_TEMP_OFFSET;
    m_config.humOffset      = Settings::DEFAULT_HUM_OFFSET;
    m_config.setpoint       = Settings::DEFAULT_SETPOINT;
    m_config.tempAlarmHigh  = Settings::DEFAULT_TEMP_ALARM_HIGH;
    m_config.tempAlarmLow   = Settings::DEFAULT_TEMP_ALARM_LOW;
    m_config.humSetpointOn  = Settings::DEFAULT_HUM_SETPOINT_ON;
    m_config.humSetpointOff = Settings::DEFAULT_HUM_SETPOINT_OFF;
    m_config.humAlarmHigh   = Settings::DEFAULT_HUM_ALARM_HIGH;
    m_config.humAlarmLow    = Settings::DEFAULT_HUM_ALARM_LOW;
    m_config.turnInterval   = Settings::DEFAULT_TURN_INTERVAL;
    m_config.turnDuration   = Settings::DEFAULT_TURN_DURATION;
    m_config.controllerType = static_cast<uint8_t>(Settings::DEFAULT_CONTROLLER);
    m_config.kp             = Settings::DEFAULT_KP;
    m_config.ki             = Settings::DEFAULT_KI;
    m_config.kd             = Settings::DEFAULT_KD;
    m_config.hysteresis     = Settings::DEFAULT_HYSTERESIS;
    m_incubationDays        = 0;
    m_dirty                 = true;
    log_w("Config factory reset");
}

SystemConfig& ConfigManager::config() {
    return m_config;
}

const SystemConfig& ConfigManager::config() const {
    return m_config;
}

uint32_t ConfigManager::incubationDays() const {
    return m_incubationDays;
}

void ConfigManager::setIncubationDays(uint32_t days) {
    if (m_incubationDays != days) {
        m_incubationDays = days;
        m_dirty = true;
    }
}

bool ConfigManager::isDirty() const {
    return m_dirty;
}

bool ConfigManager::saveIfDirty() {
    if (m_dirty) {
        return save();
    }
    return true;
}

bool ConfigManager::writeToFile(const char* path) {
    File file = SPIFFS.open(path, "w");
    if (!file) {
        log_e("Failed to open %s for writing", path);
        return false;
    }

    JsonDocument doc;
    doc["version"]          = CONFIG_VERSION;
    doc["ssid"]             = m_config.ssid;
    doc["password"]         = m_config.password;
    doc["static_ip"]        = m_config.staticIP;
    doc["static_gateway"]   = m_config.staticGateway;
    doc["static_netmask"]   = m_config.staticNetmask;
    doc["use_static_ip"]    = m_config.useStaticIP;
    doc["mqtt_server"]      = m_config.mqttServer;
    doc["mqtt_port"]        = m_config.mqttPort;
    doc["mqtt_user"]        = m_config.mqttUser;
    doc["mqtt_password"]    = m_config.mqttPassword;
    doc["mqtt_tls"]         = m_config.mqttUseTLS;
    doc["mqtt_cert"]        = m_config.mqttCert;
    doc["temp_offset"]      = m_config.tempOffset;
    doc["hum_offset"]       = m_config.humOffset;
    doc["setpoint"]         = m_config.setpoint;
    doc["temp_alarm_high"]  = m_config.tempAlarmHigh;
    doc["temp_alarm_low"]   = m_config.tempAlarmLow;
    doc["hum_on"]           = m_config.humSetpointOn;
    doc["hum_off"]          = m_config.humSetpointOff;
    doc["hum_alarm_high"]   = m_config.humAlarmHigh;
    doc["hum_alarm_low"]    = m_config.humAlarmLow;
    doc["turn_interval"]    = m_config.turnInterval;
    doc["turn_duration"]    = m_config.turnDuration;
    doc["controller_type"]  = m_config.controllerType;
    doc["kp"]               = m_config.kp;
    doc["ki"]               = m_config.ki;
    doc["kd"]               = m_config.kd;
    doc["hysteresis"]       = m_config.hysteresis;
    doc["incubation_days"]  = m_incubationDays;

    size_t bytes = serializeJson(doc, file);
    file.close();

    size_t expected = measureJson(doc);
    if (bytes != expected) {
        log_e("Write size mismatch: %u vs %u — disk full?", bytes, expected);
        SPIFFS.remove(path);
        return false;
    }

    return true;
}

bool ConfigManager::readFromFile(const char* path, JsonDocument& doc) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        log_e("JSON parse error: %s (%s)", error.c_str(), path);
        return false;
    }

    return true;
}

void ConfigManager::validateAndClamp() {
    m_config.tempOffset     = constrain(m_config.tempOffset, -10.0f, 10.0f);
    m_config.humOffset      = constrain(m_config.humOffset, -20.0f, 20.0f);
    m_config.setpoint       = constrain(m_config.setpoint, 20.0f, 60.0f);
    m_config.tempAlarmHigh  = constrain(m_config.tempAlarmHigh, m_config.setpoint, 60.0f);
    m_config.tempAlarmLow   = constrain(m_config.tempAlarmLow, 0.0f, m_config.setpoint);
    m_config.humSetpointOn  = constrain(m_config.humSetpointOn, 0.0f, 100.0f);
    m_config.humSetpointOff = constrain(m_config.humSetpointOff, m_config.humSetpointOn, 100.0f);
    m_config.humAlarmHigh   = constrain(m_config.humAlarmHigh, 0.0f, 100.0f);
    m_config.humAlarmLow    = constrain(m_config.humAlarmLow, 0.0f, 100.0f);
    m_config.turnInterval   = constrain(m_config.turnInterval, (uint32_t)1, (uint32_t)1440);
    m_config.turnDuration   = constrain(m_config.turnDuration, (uint32_t)1, (uint32_t)60);
    m_config.kp             = constrain(m_config.kp, 0.0f, 1000.0f);
    m_config.ki             = constrain(m_config.ki, 0.0f, 100.0f);
    m_config.kd             = constrain(m_config.kd, 0.0f, 100.0f);
    m_config.hysteresis     = constrain(m_config.hysteresis, 0.1f, 5.0f);
}

bool ConfigManager::migrateConfig(JsonDocument& doc, uint32_t fileVersion) {
    if (fileVersion == CONFIG_VERSION) {
        return false;
    }

    if (fileVersion < 1) {
        // Add any v0 → v1 migrations here
    }

    return true;
}
