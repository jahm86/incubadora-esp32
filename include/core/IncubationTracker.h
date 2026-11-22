#pragma once

#include <Arduino.h>

// Registrador monótono de tiempo y días de incubación.
// Utiliza una base de tiempo de 64 bits para evitar desbordamientos a los 49.7 días.
class IncubationTracker {
public:
    void begin(uint32_t elapsedSeconds);
    void reset();

    uint32_t elapsedSeconds() const;
    uint32_t days() const;

private:
    uint32_t m_baseElapsedSeconds = 0;
    int64_t  m_startUs            = 0;
};
