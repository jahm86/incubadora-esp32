#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config/pins.h"

struct Tone {
    uint8_t time;      // time=0 is 20ms, time=1 is 40ms, ..., time=254 is 5100ms, time=255 is forever
    uint8_t frequency; // frequency=0 is 0Hz (Buzzer off), frequency=1 is 75Hz, ..., frequency=255 is 19125Hz
};

struct BufferTone {
    Tone* tone;        // Pointer to first tone
    uint8_t t_size;    // Number of tones
    bool repeat;       // If repeat from first to last tone forever
};

class Buzzer {
public:
    void begin();
    void play(BufferTone buffer);
    void play(const Tone* tones, uint8_t count, bool repeat);
    void stop();
    void update();

private:
    struct Command {
        const Tone* tones;
        uint8_t count;
        bool repeat;
        bool stop;
    };

    static constexpr uint8_t CHANNEL = 1;
    static constexpr uint32_t FOREVER_MS = 0xFFFFFFFF;

    static uint32_t toneDurationMs(const Tone& tone);
    void setFrequency(uint16_t freqHz);
    void startBuffer(const Command& cmd);
    void nextTone();
    void silence();

    QueueHandle_t m_queue = nullptr;
    const Tone* m_tones = nullptr;
    uint8_t m_count = 0;
    bool m_repeat = false;
    uint8_t m_index = 0;
    uint32_t m_toneStart = 0;
    bool m_playing = false;

    static void task(void* param);
};
