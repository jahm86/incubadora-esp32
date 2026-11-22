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
# Compilar firmware ESP32
pio run

# Ejecutar tests unitarios nativos (en PC, ultrarrápido sin hardware)
pio test -e native

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
│   ├── alarm/
│   │   └── AlarmManager.h
│   ├── config/
│   │   ├── FieldRegistry.h
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
│   │   ├── ConfigManager.h
│   │   └── IncubationTracker.h
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
├── src/
│   ├── AHT30.cpp
│   ├── AlarmManager.cpp
│   ├── Buzzer.cpp
│   ├── ConfigManager.cpp
│   ├── DisplayManager.cpp
│   ├── EggTray.cpp
│   ├── FieldRegistry.cpp
│   ├── Heater.cpp
│   ├── Humidifier.cpp
│   ├── HysteresisController.cpp
│   ├── IncubationTracker.cpp
│   ├── LADRCController.cpp
│   ├── main.cpp
│   ├── MenuSystem.cpp
│   ├── MqttManager.cpp
│   ├── PIDController.cpp
│   ├── RotaryEncoder.cpp
│   ├── WebServer.cpp
│   └── WiFiManager.cpp
└── test/
    ├── local/
    └── test_controllers/
        └── test_controllers.cpp
```

## Estado actual
- **ConfigManager & FieldRegistry**: `FieldRegistry` como Single Source of Truth para rangos, labels, validación centralizada y dependencias cruzadas de los 22 campos; persistencia atómica con backup en SPIFFS.
- **IncubationTracker**: Conteo monótono de tiempo y días con base de 64 bits (`esp_timer_get_time()`), inmune al overflow de 49.7 días; persistencia periódica de segundos en SPIFFS.
- **AlarmManager**: Módulo autónomo desacoplado que gestiona 4 melodías diferenciadas, prioridades fijas (Temp Alta > Temp Baja > Hum Alta > Hum Baja), snooze de 10 min y publicación a MQTT vía hook.
- **DisplayManager Anti-flickering**: Renderizado sin parpadeo mediante transiciones limpias de pantalla (`switchScreen`), `setTextPadding()` para sobreescritura directa en celda y *dirty tracking* por fila/componente.
- **Baja latencia en UI**: En `uiTask`, las acciones del encoder (giro, clic, navegación) disparan actualización inmediata en el ciclo de 10 ms sin esperar el refresco periódico de 250 ms.
- **Red resiliente no bloqueante**: `WiFiManager` y `MqttManager` con reconexión asíncrona periódica (backoff de 10s a 30s) sin congelar la tarea de control térmico; soporte para cambios de credenciales en caliente vía `AppState`.
- **Pruebas unitarias nativas**: Entorno `[env:native]` con Unity para ejecución ultrarrápida de tests de algoritmos térmicos (`PIDController`, `HysteresisController`, `LADRCController`) en PC mediante `pio test -e native`.
- **Control térmico**: Hysteresis, PID y LADRC implementados, testeados matemáticamente y seleccionables en runtime.

## Arquitectura de tareas (FreeRTOS)
- **loopTask** (core 1, Arduino `loop()`): sensor, control térmico, alarmas (`AlarmManager`), volteo, días (`IncubationTracker`), reconexión de red (`WiFiManager`/`MqttManager`), saves y peticiones atómicas
- **uiTask** (core 0): encoder, máquina de menú, renderizado sin parpadeo de pantalla (`DisplayManager`), snooze de alarma (10 min)
- **buzzerTask** (core 0): reproduce melodías (`Tone`/`BufferTone`) por cola FreeRTOS, sin bloquear el loop
- **Watchdog**: `esp_task_wdt_init(10, true)`; cada task se suscribe (`esp_task_wdt_add(NULL)`) y alimenta (`esp_task_wdt_reset()`)
- **Estado compartido (`AppState`)**: mutex para `SystemConfig`, banderas atómicas de solicitud (`requestConfigSave`, `requestControllerInit`, `requestWifiReconnect`, etc.) para canalizar cambios hacia `loopTask` sin condiciones de carrera.

## Menú y edición
- Menú jerárquico (Root → Control / Temperatura / Humedad / Volteo / Sistema) con scroll en pantalla; `VISIBLE_ROWS=8` por pantalla e indicador "v scroll" cuando excede ese número
- Edit de valores: girar = ajustar valor candidato, presionar = confirmar. Se valida y acota contra `FieldRegistry`
- Servidor Web y Buzzer se editan vía `FieldRegistry` con la misma dinámica que el resto de campos
- Al volver de un submenú se restaura la selección/scroll previo
- La config se guarda solo en eventos discretos (salida de edición con cambios, MQTT, web), nunca en cada tick del encoder
- `m_setpoint` vive en `IController` (base); `compute(float input)` usa el setpoint almacenado
