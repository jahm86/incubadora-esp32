#include <Arduino.h>
#include "output/EggTray.h"

void EggTray::begin() {
    pinMode(PIN_EGG_TRAY, OUTPUT);
    off();
}

void EggTray::on() {
    digitalWrite(PIN_EGG_TRAY, HIGH);
    m_active = true;
}

void EggTray::off() {
    digitalWrite(PIN_EGG_TRAY, LOW);
    m_active = false;
}
