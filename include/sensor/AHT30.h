#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "config/pins.h"
#include "types.h"

class AHT30 {
public:
    void begin();
    SensorData read();
    SensorData readWithOffset(float tempOffset, float humOffset);

private:
    Adafruit_AHTX0 m_aht;
};
