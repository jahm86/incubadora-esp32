# Incubadora ESP32

Controlador de incubadora para ESP32 con sensor AHT30, pantalla TFT ST7789, encoder rotativo, MQTT/TLS y servidor web de configuración.

## Hardware

| Componente | Pines |
|---|---|
| Sensor AHT30 (I2C) | SDA 21, SCL 22 |
| TFT ST7789 SPI | MOSI 23, SCLK 18, CS 5, DC 17, RST 16, BL 19 |
| Encoder KY-040 | CLK 32, DT 33, SW 25 |
| Buzzer | GPIO 26 |
| Calefactor (PWM) | GPIO 27 |
| Humidificador | GPIO 14 |
| Motor volteo | GPIO 13 |

Ver `include/config/pins.h` para más detalle. El ventilador va conectado directo a la fuente (siempre encendido, sin control por el ESP32); esto podría cambiar si se encuentra una justificación creíble para manipularlo desde el micro.

## Funcionalidades

- Medición de temperatura y humedad con offset configurable
- Control PID, Hysteresis o LADRC para calefactor (seleccionable en menú)
- Control ON/OFF de humidificador por histéresis
- Volteo de huevos programable (intervalo + duración) con "voltear ahora" desde el menú
- Conteo de días de incubación (persistente ante reinicios)
- Pantalla TFT con menú jerárquico de configuración (navegación por encoder)
- MQTT con soporte TLS para monitoreo y configuración remota (probado con broker HiveMQ)
- Servidor web de configuración (AP al arrancar; en modo STA se activa desde el menú)
- Alertas sonoras diferenciadas por condición con snooze de 10 min al pulsar el botón
- Feedback sonoro del encoder (clic/tac)
- Menú Sistema: Servidor Web y Buzzer ON/OFF, reiniciar, reset de días y restauración de fábrica (con confirmación)
- Persistencia de configuración en SPIFFS (JSON + backup)

## Compilación y subida

```bash
# Compilar
pio run

# Subir firmware
pio run --target upload

# Subir sistema de archivos (web)
pio run --target uploadfs

# Subir todo
pio run --target upload && pio run --target uploadfs
```

**Nota:** Se usa la partición `huge_app.csv` (3MB app + 1MB SPIFFS).

## Build flags relevantes

- `CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_INFO` — Habilita logs por Serial (`log_i`, `log_e`, `log_w`). Para debug más fino cambiar a `ARDUHAL_LOG_LEVEL_DEBUG` (incluye `log_d`).
- `TONE_TEST_ON` — Compila el tópico de prueba de tonos `incubadora/test/tone` (ver sección MQTT). Por defecto viene comentado en `platformio.ini`.

## Configuración web

Al iniciar sin conexión WiFi configurada (o si falla), el ESP32 crea el AP:

- **SSID:** `Incubadora-AP`
- **Password:** `config1234`
- **IP:** `192.168.4.1`

Conectarse a la red, abrir `http://192.168.4.1` y configurar SSID local y broker MQTT. En modo STA el servidor web se puede activar/desactivar desde el menú **Sistema → Servidor Web** (cambio no persistente).

## Tópicos MQTT

Prefijo base: `incubadora/`. Los tópicos se definen en `include/config/mqtt_topics.h`.

**Estado (publicados periódicamente)** — `incubadora/status/temperatura`, `incubadora/status/humedad`, `incubadora/status/dias`, `incubadora/status/alarma`.

**Configuración (suscritos, se cambia un parámetro publicando su valor numérico)** — `incubadora/config/setpoint`, `incubadora/config/hum_on`, `incubadora/config/hum_off`, `incubadora/config/temp_offset`, `incubadora/config/hum_offset`, `incubadora/config/temp_alarm_high`, `incubadora/config/temp_alarm_low`, `incubadora/config/hum_alarm_high`, `incubadora/config/hum_alarm_low`, `incubadora/config/turn_interval`, `incubadora/config/turn_duration`, `incubadora/config/controller_type`, `incubadora/config/kp`, `incubadora/config/ki`, `incubadora/config/kd`, `incubadora/config/hysteresis`, `incubadora/config/b0`, `incubadora/config/wc`, `incubadora/config/wo`.

**Consulta de valores (request/response)** — publicar el nombre del parámetro en `incubadora/config/request`; el equipo responde en `incubadora/config/response` con `[nombre]:[valor]` (o `error(desconocido):[nombre]`).

**Comandos** — reinicio del equipo: `incubadora/cmd/restart`.

**Prueba de tonos (solo con `TONE_TEST_ON`)** — `incubadora/test/tone`. Formato: `(t1,f1;t2,f2;...)repeat`, donde cada `t` es la duración (0-255; `time=255` es infinito, tiempo base 20 ms) y cada `f` la frecuencia (0-255; `frecuencia = f * 50 Hz`), hasta 64 tonos. El sufijo `repeat` es `t`, `f` o vacío (sin repetir). Ejemplo: `(4,40;4,25;4,2)f`. Tiempo y frecuencia base son ajustables mediante `BASE_TIME_MS` y `BASE_FREQ_HZ` (definidos en `include/output/Buzzer.h`).

## Dependencias (PlatformIO)

| Librería | Propósito |
|---|---|
| `bodmer/TFT_eSPI` | Controlador de pantalla |
| `theelims/PsychicMqttClient` | Cliente MQTT asíncrono con TLS |
| `hoeken/PsychicHttp` | Servidor web asíncrono |
| `adafruit/Adafruit AHTX0` | Sensor AHT30 (I2C) |
| `maffooclock/ESP32RotaryEncoder` | Encoder rotativo |
| `bblanchon/ArduinoJson` | Persistencia JSON |

## Licencia

GNU General Public License v3.0 — ver [LICENSE](LICENSE).
