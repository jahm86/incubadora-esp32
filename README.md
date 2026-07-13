# Incubadora ESP32

Controlador de incubadora para ESP32 con sensor AM2120, pantalla TFT ST7789, encoder rotativo, MQTT/TLS y servidor web de configuración.

## Hardware

| Componente | Pin |
|---|---|
| Sensor AM2120 | GPIO 4 |
| TFT ST7789 SPI | MOSI 23, SCLK 18, CS 5, DC 17, RST 16, BL 19 |
| Encoder KY-040 | CLK 32, DT 33, SW 34 |
| Buzzer | GPIO 14 |
| Calefactor (PWM) | GPIO 26 |
| Humidificador | GPIO 27 |
| Motor volteo | GPIO 13 |

Ver `include/config/pins.h` para más detalle.

## Funcionalidades

- Medición de temperatura y humedad con offset configurable
- Control PID o ARDC para calefactor (seleccionable en menú)
- Control ON/OFF de humidificador por histéresis
- Volteo de huevos programable (intervalo + duración)
- Conteo de días de incubación
- Pantalla TFT con menú de configuración (navegación por encoder)
- MQTT con soporte TLS para monitoreo remoto
- Servidor web en modo AP para configuración inicial
- Alertas por buzzer y MQTT
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

## Configuración web

Al iniciar sin conexión WiFi configurada, el ESP32 crea el AP:

- **SSID:** `Incubadora-AP`
- **Password:** `config1234`
- **IP:** `192.168.4.1`

Conectarse a la red, abrir `http://192.168.4.1` y configurar SSID local y broker MQTT.

## Dependencias (PlatformIO)

| Librería | Propósito |
|---|---|
| `bodmer/TFT_eSPI` | Controlador de pantalla |
| `theelims/PsychicMqttClient` | Cliente MQTT asíncrono con TLS |
| `hoeken/PsychicHttp` | Servidor web asíncrono |
| `adafruit/DHT sensor library` | Sensor AM2120 |
| `maffooclock/ESP32RotaryEncoder` | Encoder rotativo |
| `bblanchon/ArduinoJson` | Persistencia JSON |

## Licencia

GNU General Public License v3.0 — ver [LICENSE](LICENSE).
