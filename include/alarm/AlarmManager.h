#pragma once

#include <Arduino.h>
#include <functional>
#include "types.h"
#include "output/Buzzer.h"

// Gestiona la evaluación periódica de umbrales de alarma, selección de melodías,
// silenciamiento temporal (snooze) y notificación hacia MQTT y salidas.
class AlarmManager {
public:
    using PublishHook = std::function<void(const char* topic, const char* payload)>;

    void begin(Buzzer* buzzer, OutputState* outputs);
    void setPublishHook(PublishHook hook) { m_publishHook = hook; }

    void evaluate(const SensorData& sensor, const SystemConfig& config);
    void snooze(uint32_t durationMs = 600000UL);

    bool isActive() const { return m_active; }
    const char* message() const { return m_message; }
    uint8_t mask() const { return m_mask; }

private:
    const BufferTone* bufferForMask(uint8_t mask) const;

    Buzzer*      m_buzzer      = nullptr;
    OutputState* m_outputs     = nullptr;
    PublishHook  m_publishHook = nullptr;

    volatile bool     m_active = false;
    volatile uint32_t m_snoozedUntil = 0;
    char              m_message[64] = {0};
    uint8_t           m_mask = 0;
    const BufferTone* m_playingBuf = nullptr;
};
