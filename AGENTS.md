# AGENTS.md - Incubadora ESP32

## Stack tecnológico
- **Framework**: Arduino (C++) sobre PlatformIO
- **Display**: TFT_eSPI para ST7789 240x280 SPI
- **MQTT**: PsychicMqttClient (asíncrono, con soporte TLS)
- **Web Server**: PsychicHttp (asíncrono, sirve archivos desde SPIFFS)
- **Sensor**: AHT30 (I2C, usar Adafruit AHTX0 library)
- **Encoder**: ESP32RotaryEncoder (por MaffooClock)
- **Persistencia**: SPIFFS + JSON con ArduinoJson
- **Control térmico**: Clase abstracta IController, implementaciones PID, Hysteresis y LADRC

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
- Headers en `include/`, organizados por módulo; implementaciones en `src/*.cpp` (no header-only, salvo `AppState`/`IController` que son contenedores/interfaces)
- Archivos web estáticos en `data/`
- Sin comentarios triviales; sí agregar comentarios de grupo breves en structs, enums y constexpr/const complejos para que los humanos entiendan el uso (p.ej. `MqttTopics`, `Tone`/`BufferTone`). No comentar cada elemento, solo el grupo o su uso
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
│   │   ├── HysteresisController.h
│   │   ├── IController.h
│   │   ├── LADRCController.h
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
│   │   └── AHT30.h
│   ├── types.h
│   └── web/
│       └── WebServer.h
└── src/
    ├── AHT30.cpp
    ├── Buzzer.cpp
    ├── ConfigManager.cpp
    ├── DisplayManager.cpp
    ├── EggTray.cpp
    ├── Heater.cpp
    ├── Humidifier.cpp
    ├── HysteresisController.cpp
    ├── LADRCController.cpp
    ├── main.cpp
    ├── MenuSystem.cpp
    ├── MqttManager.cpp
    ├── PIDController.cpp
    ├── RotaryEncoder.cpp
    ├── WebServer.cpp
    └── WiFiManager.cpp
```

## Estado actual
- ConfigManager completo: saves atómicos, backup, validación, factory reset, versionado
- Persistencia de días de incubación: se guarda `incubation_elapsed_s` cada 10 min (sobrevive reinicios)
- WiFi AP + web server funcionales, con modo STA cuando se configura SSID
- Web server: arranca en modo AP al bootear; en STA se activa/desactiva en runtime desde el menú Sistema (campo "Servidor Web", no persistente)
- Display TFT ST7789, encoder rotativo y sensor AHT30 probados en hardware real
- Controladores: Hysteresis, PID y LADRC implementados y seleccionables
- MQTT probado con broker real HiveMQ, incluyendo comunicación con TLS/SSL (certificado de HiveMQ) y usuario/contraseña
- Buzzer: toggle persistente (`buzzer_enabled` en JSON), 4 melodías de alarma diferenciadas con prioridad fija (Temp Alta > Temp Baja > Hum Alta > Hum Baja, vía `alarmMask`), snooze de 10 min al presionar el botón y feedback sonoro (clic/tac) del encoder
- Menú Sistema con confirmaciones para reiniciar, reset de días, restauración de fábrica y volteo manual ("Voltear Ahora")

## Arquitectura de tareas (FreeRTOS)
- **loopTask** (core 1, Arduino `loop()`): sensor, control térmico, alarmas (solo evalúa y encola), volteo, días de incubación, MQTT, saves
- **uiTask** (core 0): encoder, máquina de menú, render del display, dismiss/snooze de alarma (10 min)
- **buzzerTask** (core 0): reproduce melodías (estructuras `Tone`/`BufferTone`) por cola FreeRTOS, no bloquea el loop
- **Watchdog**: `esp_task_wdt_init(10, true)`; cada task se suscribe (`esp_task_wdt_add(NULL)`) y alimenta (`esp_task_wdt_reset()`)
- Estado compartido (`AppState`): el control lee sensor/config, la UI escribe config. Escrituras de 32 bits atómicas en ESP32 → races benignas aceptadas

## Menú y edición
- Menú jerárquico (Root → Control / Temperatura / Humedad / Volteo / Sistema) con scroll en pantalla; `VISIBLE_ROWS=8` por pantalla y un indicador "v scroll" arriba a la derecha cuando la lista excede ese número (p.ej. Control)
- Edit de valores: girar = ajustar un valor candidato, presionar = salir de la edición. **No hay cambios en vivo**: al salir (`onExitEdit`) se compara el valor con el original (`g_editOriginal`/`g_editCurrent`) y solo si cambió se aplica `def.set()`, se reinicia el controlador (setpoint/controller_type) y se guarda config
- Servidor Web y Buzzer se editan por enum OFF/ON vía `fieldDefs()`, con la misma dinámica que el resto de campos (sin labels dinámicos)
- Al volver de un submenú se restaura la selección/scroll previo (`m_prevSelected`/`m_prevScroll`)
- La config se guarda solo en eventos discretos (salida de edición con cambios, MQTT, web), no en cada tick del encoder
- `m_setpoint` vive en `IController` (base); `compute(float input)` usa el setpoint almacenado
