#include "alarm/AlarmManager.h"
#include <string.h>

namespace {

const Tone kAlarmTempHigh[] = {{39, 19}, {9, 0}};
const Tone kAlarmTempLow[]  = {{44, 11}, {4, 0}};
const Tone kAlarmHumHigh[]  = {{4, 40}, {4, 25}, {4, 2}};
const Tone kAlarmHumLow[]   = {{4, 20}, {4, 15}, {4, 1}};

const BufferTone kAlarmTempHighBuf = {const_cast<Tone*>(kAlarmTempHigh), sizeof(kAlarmTempHigh) / sizeof(Tone), true};
const BufferTone kAlarmTempLowBuf  = {const_cast<Tone*>(kAlarmTempLow),  sizeof(kAlarmTempLow)  / sizeof(Tone), true};
const BufferTone kAlarmHumHighBuf  = {const_cast<Tone*>(kAlarmHumHigh),  sizeof(kAlarmHumHigh)  / sizeof(Tone), true};
const BufferTone kAlarmHumLowBuf   = {const_cast<Tone*>(kAlarmHumLow),   sizeof(kAlarmHumLow)   / sizeof(Tone), true};

} // namespace

void AlarmManager::begin(Buzzer* buzzer, OutputState* outputs) {
    m_buzzer = buzzer;
    m_outputs = outputs;
    m_active = false;
    m_snoozedUntil = 0;
    m_mask = 0;
    m_playingBuf = nullptr;
    m_message[0] = '\0';
}

const BufferTone* AlarmManager::bufferForMask(uint8_t mask) const {
    if (mask & AlarmFlag::AlarmTempHigh) return &kAlarmTempHighBuf;
    if (mask & AlarmFlag::AlarmTempLow)  return &kAlarmTempLowBuf;
    if (mask & AlarmFlag::AlarmHumHigh)  return &kAlarmHumHighBuf;
    return &kAlarmHumLowBuf;
}

void AlarmManager::snooze(uint32_t durationMs) {
    m_snoozedUntil = millis() + durationMs;
    m_active = false;
    if (m_outputs) {
        m_outputs->buzzerActive = false;
    }
    if (m_buzzer) {
        m_buzzer->stop();
    }
}

void AlarmManager::evaluate(const SensorData& sensor, const SystemConfig& config) {
    uint8_t mask = 0;
    char msg[64] = {0};

    if (sensor.valid) {
        float temp = sensor.temperature;
        float hum  = sensor.humidity;

        if (temp > config.tempAlarmHigh) {
            mask |= AlarmFlag::AlarmTempHigh;
            snprintf(msg, sizeof(msg), "Temp ALTA: %.1f C", temp);
        } else if (temp < config.tempAlarmLow) {
            mask |= AlarmFlag::AlarmTempLow;
            snprintf(msg, sizeof(msg), "Temp BAJA: %.1f C", temp);
        }

        if (hum > config.humAlarmHigh) {
            mask |= AlarmFlag::AlarmHumHigh;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum ALTA: %.1f%%", hum);
        } else if (hum < config.humAlarmLow) {
            mask |= AlarmFlag::AlarmHumLow;
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), " Hum BAJA: %.1f%%", hum);
        }
    }

    bool shouldAlarm = (mask != 0) && (millis() >= m_snoozedUntil);

    if (shouldAlarm && !m_active) {
        m_active = true;
        m_mask = mask;
        strncpy(m_message, msg, sizeof(m_message) - 1);
        m_message[sizeof(m_message) - 1] = '\0';
        if (m_outputs) m_outputs->buzzerActive = true;
        m_playingBuf = bufferForMask(mask);
        if (m_buzzer) m_buzzer->play(*m_playingBuf);
        if (m_publishHook) {
            m_publishHook("incubadora/status/alarma", m_message);
        }
    } else if (shouldAlarm && m_active) {
        const BufferTone* buf = bufferForMask(mask);
        if (buf != m_playingBuf) {
            m_playingBuf = buf;
            if (m_buzzer) m_buzzer->play(*buf);
        }
        if (mask != m_mask) {
            m_mask = mask;
            strncpy(m_message, msg, sizeof(m_message) - 1);
            m_message[sizeof(m_message) - 1] = '\0';
        }
    } else if (!shouldAlarm && m_active) {
        m_active = false;
        m_mask = 0;
        m_playingBuf = nullptr;
        if (m_outputs) m_outputs->buzzerActive = false;
        if (m_buzzer) m_buzzer->stop();
    }
}
