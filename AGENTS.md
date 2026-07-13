# AGENTS.md - Incubadora ESP32

## Stack tecnológico
- **Framework**: Arduino (C++) sobre PlatformIO
- **Display**: TFT_eSPI para ST7789 240x280 SPI
- **MQTT**: PsychicMqttClient (asíncrono, con soporte TLS)
- **Web Server**: PsychicHttp (asíncrono, sirve archivos desde SPIFFS)
- **Sensor**: AM2120 (protocolo 1-wire compatible DHT, usar Adafruit DHT library)
- **Encoder**: ESP32RotaryEncoder (por MaffooClock)
- **Persistencia**: SPIFFS + JSON con ArduinoJson
- **Control térmico**: Clase abstracta IController, implementaciones PID y ARDC

## Comandos útiles
```bash
# Compilar
pio run

# Subir firmware
pio run --target upload

# Subir SPIFFS
pio run --target uploadfs

# Monitor serial
pio device monitor

# Compilar y subir todo (se usa partición huge_app: 3MB app + 1MB SPIFFS)
pio run --target upload && pio run --target uploadfs
```

## Build flags (debug)
```bash
-D CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_INFO   # log_i, log_w, log_e habilitados
-D CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_DEBUG  # también log_d
```

Los macros `log_i`, `log_d`, `log_w`, `log_e` se usan en vez de `Serial.printf`. Son de `esp32-hal-log.h` y no llevan tag:
```cpp
log_i("Boot OK");                     // formato simple
log_d("Valor: %d", val);              // con argumentos
log_e("Fallo: %s", error.c_str());    // strings
```

## Reglas estrictas (DO NOT)
- **No** hacer push al remoto sin confirmación explícita del desarrollador para ese push específico
- **No** commitear si hay errores de compilación

## Convenciones de código
- Headers en `include/`, organizados por módulo
- Archivos web estáticos en `data/`
- Sin comentarios en código a menos que sea necesario
- Nombres de clases en PascalCase, métodos en camelCase
- Constantes en UPPER_CASE
- Usar `#pragma once` en vez de include guards

## Comunicación serial con el ESP32
Si el agente intenta leer el puerto serial del ESP32 mediante scripts de Python y no obtiene respuesta tras varios intentos, debe pedir ayuda al desarrollador para que ejecute `pio device monitor` o lea el serial manualmente. El agente no debe insistir con intentos automáticos repetitivos.

## Estructura del proyecto
```
.
├── platformio.ini
├── data/
│   ├── index.html
│   ├── script.js
│   └── style.css
├── include/
│   ├── config/
│   │   ├── mqtt_topics.h
│   │   ├── pins.h
│   │   └── settings.h
│   ├── control/
│   │   ├── ARDCController.h
│   │   ├── IController.h
│   │   └── PIDController.h
│   ├── core/
│   │   ├── AppState.h
│   │   └── ConfigManager.h
│   ├── display/
│   │   ├── DisplayManager.h
│   │   └── MenuSystem.h
│   ├── input/
│   │   └── RotaryEncoder.h
│   ├── network/
│   │   ├── MqttManager.h
│   │   └── WiFiManager.h
│   ├── output/
│   │   ├── Buzzer.h
│   │   ├── EggTray.h
│   │   ├── Heater.h
│   │   └── Humidifier.h
│   ├── sensor/
│   │   └── AM2120.h
│   ├── types.h
│   └── web/
│       └── WebServer.h
└── src/
    ├── ConfigManager.cpp
    └── main.cpp
```

## Estado actual
- ConfigManager completo: saves atómicos, backup, validación, factory reset, versionado
- WiFi AP + web server funcionales, con modo STA cuando se configura SSID
- Logs por Serial funcionales (requiere `CORE_DEBUG_LEVEL` correcto)
- Pendiente: probar sensor AM2120, display TFT y encoder en hardware real
