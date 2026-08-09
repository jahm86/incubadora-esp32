#include <Arduino.h>
#include "output/Buzzer.h"
#include <esp_task_wdt.h>

void Buzzer::begin() {
    ledcSetup(CHANNEL, 1000, 8);
    ledcAttachPin(PIN_BUZZER, CHANNEL);
    silence();
    m_queue = xQueueCreate(4, sizeof(Command));
    xTaskCreatePinnedToCore(Buzzer::task, "buzzerTask", 2048, this, 1, nullptr, 0);
}

void Buzzer::play(BufferTone buffer) {
    if (!m_enabled) {
        return;
    }
    Command cmd = {buffer.tone, buffer.t_size, buffer.repeat, false};
    xQueueSend(m_queue, &cmd, portMAX_DELAY);
}

void Buzzer::play(const Tone* tones, uint8_t count, bool repeat) {
    if (!m_enabled) {
        return;
    }
    Command cmd = {tones, count, repeat, false};
    xQueueSend(m_queue, &cmd, portMAX_DELAY);
}

void Buzzer::stop() {
    Command cmd = {nullptr, 0, false, true};
    xQueueSend(m_queue, &cmd, portMAX_DELAY);
}

bool Buzzer::playNonBlocking(const Tone* tones, uint8_t count, bool repeat) {
    if (!m_enabled) {
        return false;
    }
    Command cmd = {tones, count, repeat, false};
    return xQueueSend(m_queue, &cmd, 0) == pdTRUE;
}

void Buzzer::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!m_enabled) {
        stop();
    }
}

void Buzzer::task(void* param) {
    Buzzer* bz = static_cast<Buzzer*>(param);
    esp_task_wdt_add(NULL);
    while (true) {
        bz->update();
    }
}

void Buzzer::update() {
    esp_task_wdt_reset();

    Command cmd;
    if (xQueueReceive(m_queue, &cmd, 0) == pdTRUE) {
        if (cmd.stop) {
            silence();
            m_playing = false;
        } else {
            startBuffer(cmd);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        return;
    }

    if (m_playing && m_index < m_count) {
        uint32_t duration = toneDurationMs(m_tones[m_index]);
        if (duration != FOREVER_MS && (millis() - m_toneStart) >= duration) {
            nextTone();
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

uint32_t Buzzer::toneDurationMs(const Tone& tone) {
    if (tone.time == 255) {
        return FOREVER_MS;
    }
    return (static_cast<uint32_t>(tone.time) + 1) * 20;
}

void Buzzer::setFrequency(uint16_t freqHz) {
    if (freqHz == 0) {
        ledcWrite(CHANNEL, 0);
        return;
    }
    ledcWriteTone(CHANNEL, freqHz);
    ledcWrite(CHANNEL, 127);
}

void Buzzer::startBuffer(const Command& cmd) {
    if (cmd.count == 0) {
        silence();
        m_playing = false;
        return;
    }
    m_tones = cmd.tones;
    m_count = cmd.count;
    m_repeat = cmd.repeat;
    m_index = 0;
    m_toneStart = millis();
    setFrequency(static_cast<uint16_t>(m_tones[0].frequency) * 60);
    m_playing = true;
}

void Buzzer::nextTone() {
    m_index++;
    if (m_index >= m_count) {
        if (m_repeat) {
            m_index = 0;
        } else {
            silence();
            m_playing = false;
            return;
        }
    }
    m_toneStart = millis();
    setFrequency(static_cast<uint16_t>(m_tones[m_index].frequency) * 75);
}

void Buzzer::silence() {
    ledcWrite(CHANNEL, 0);
}
