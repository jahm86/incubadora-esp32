#include "core/IncubationTracker.h"
#include <esp_timer.h>

void IncubationTracker::begin(uint32_t elapsedSeconds) {
    m_baseElapsedSeconds = elapsedSeconds;
    m_startUs = esp_timer_get_time();
}

void IncubationTracker::reset() {
    m_baseElapsedSeconds = 0;
    m_startUs = esp_timer_get_time();
}

uint32_t IncubationTracker::elapsedSeconds() const {
    int64_t nowUs = esp_timer_get_time();
    int64_t diffUs = nowUs - m_startUs;
    if (diffUs < 0) {
        diffUs = 0;
    }
    return m_baseElapsedSeconds + static_cast<uint32_t>(diffUs / 1000000LL);
}

uint32_t IncubationTracker::days() const {
    return elapsedSeconds() / 86400UL;
}
