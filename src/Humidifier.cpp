#include <Arduino.h>
#include "output/Humidifier.h"

void Humidifier::begin() {
    pinMode(PIN_HUMIDIFIER, OUTPUT);
    off();
}

void Humidifier::on() {
    digitalWrite(PIN_HUMIDIFIER, HIGH);
    m_active = true;
}

void Humidifier::off() {
    digitalWrite(PIN_HUMIDIFIER, LOW);
    m_active = false;
}
