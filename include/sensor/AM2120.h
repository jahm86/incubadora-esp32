#pragma once

#include <Arduino.h>
#include <DHT.h>
#include "config/pins.h"
#include "types.h"

class AM2120 {
public:
    AM2120() : m_dht(PIN_SENSOR_DATA, DHT22) {}

    void begin() {
        m_dht.begin();
    }

    SensorData read() {
        SensorData data;
        data.temperature = m_dht.readTemperature();
        data.humidity    = m_dht.readHumidity();
        data.valid       = !isnan(data.temperature) && !isnan(data.humidity);
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
    DHT m_dht;
};
