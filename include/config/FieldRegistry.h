#pragma once

#include <Arduino.h>
#include "types.h"
#include "display/MenuSystem.h"

// Metadatos y funciones de acceso para campos de configuración numéricos y enums.
struct FieldDef {
    MenuField   field;
    const char* key;        // Clave JSON y sub-tópico MQTT ("" para campos virtuales)
    const char* label;      // Etiqueta en pantalla TFT
    const char* unit;       // Unidad física ("C", "%", "min", "s", "")
    float       step;       // Paso de ajuste del encoder
    float       min;        // Límite inferior
    float       max;        // Límite superior
    bool        isInt;      // Es entero
    bool        isEnum;     // Es selector de opciones
    const char* const* enumNames; // Nombres de opciones si isEnum es true

    float (*get)(const SystemConfig& cfg);
    void  (*set)(SystemConfig& cfg, float value);
};

class FieldRegistry {
public:
    static const FieldDef* get(MenuField field);
    static const FieldDef* findByKey(const char* key);
    static const FieldDef* findByKey(const String& key);
    static const FieldDef* all();
    static constexpr size_t count() { return static_cast<size_t>(MenuField::Count); }

    // Aplica límites min/max y redondeo si es entero o enum
    static float clampValue(const FieldDef& def, float value);

    // Formatea el valor para pantalla TFT según el tipo de campo
    static void formatValue(const FieldDef& def, float value, char* buf, size_t size);

    // Aplica restricciones cruzadas entre campos (alarmas vs setpoint, hum off vs on)
    static void clampCrossDependencies(SystemConfig& cfg);
};
