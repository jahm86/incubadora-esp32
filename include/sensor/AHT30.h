#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "config/pins.h"
#include "types.h"

class AHT30 {
public:
    void begin() {
        Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
        if (!m_aht.begin()) {
            log_e("AHT30 not found on I2C bus");
        }
    }

    SensorData read() {
        SensorData data;
        sensors_event_t humidity, temp;
        if (m_aht.getEvent(&humidity, &temp)) {
            data.temperature = temp.temperature;
            data.humidity    = humidity.relative_humidity;
            data.valid       = true;
        } else {
            data.valid = false;
        }
        return data;
    }

    SensorData readWithOffset(float tempOffset, float humOffset) {
        SensorData data = read();
        if (data.valid) {
            data.temperature += tempOffset;
            data.humidity    += humOffset;
        }
        return data;
    }

private:
    Adafruit_AHTX0 m_aht;
};
