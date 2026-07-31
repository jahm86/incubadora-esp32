#include <Arduino.h>
#include "sensor/AHT30.h"

void AHT30::begin() {
    Wire.begin(PIN_SENSOR_SDA, PIN_SENSOR_SCL);
    if (!m_aht.begin()) {
        log_e("AHT30 not found on I2C bus");
    }
}

SensorData AHT30::read() {
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

SensorData AHT30::readWithOffset(float tempOffset, float humOffset) {
    SensorData data = read();
    if (data.valid) {
        data.temperature += tempOffset;
        data.humidity    += humOffset;
    }
    return data;
}
