#include <unity.h>
#include <string.h>
#include "config/FieldRegistry.h"

void setUp(void) {}
void tearDown(void) {}

void test_field_registry_count_and_all(void) {
    TEST_ASSERT_EQUAL_UINT32(22, FieldRegistry::count());
    const FieldDef* all = FieldRegistry::all();
    TEST_ASSERT_NOT_NULL(all);
}

void test_field_registry_get_by_enum(void) {
    for (size_t i = 0; i < FieldRegistry::count(); i++) {
        MenuField f = static_cast<MenuField>(i);
        const FieldDef* def = FieldRegistry::get(f);
        TEST_ASSERT_NOT_NULL(def);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(f), static_cast<uint8_t>(def->field));
        TEST_ASSERT_NOT_NULL(def->label);
        TEST_ASSERT_TRUE(strlen(def->label) > 0);
        TEST_ASSERT_TRUE(def->step > 0.0f);
        TEST_ASSERT_TRUE(def->min < def->max);
        TEST_ASSERT_NOT_NULL(def->get);
        TEST_ASSERT_NOT_NULL(def->set);
    }

    // Fuera de rango
    TEST_ASSERT_NULL(FieldRegistry::get(MenuField::Count));
    TEST_ASSERT_NULL(FieldRegistry::get(static_cast<MenuField>(99)));
}

void test_field_registry_find_by_key(void) {
    // Claves numéricas y de control
    const FieldDef* pSetpoint = FieldRegistry::findByKey("setpoint");
    TEST_ASSERT_NOT_NULL(pSetpoint);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::Setpoint), static_cast<uint8_t>(pSetpoint->field));

    const FieldDef* pKp = FieldRegistry::findByKey("kp");
    TEST_ASSERT_NOT_NULL(pKp);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::Kp), static_cast<uint8_t>(pKp->field));

    const FieldDef* pTurnInterval = FieldRegistry::findByKey("turn_interval");
    TEST_ASSERT_NOT_NULL(pTurnInterval);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::TurnInterval), static_cast<uint8_t>(pTurnInterval->field));

    const FieldDef* pController = FieldRegistry::findByKey("controller_type");
    TEST_ASSERT_NOT_NULL(pController);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::ControllerType), static_cast<uint8_t>(pController->field));

    const FieldDef* pBuzzer = FieldRegistry::findByKey("buzzer_enabled");
    TEST_ASSERT_NOT_NULL(pBuzzer);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::BuzzerEnabled), static_cast<uint8_t>(pBuzzer->field));

    // Con sobrecarga String
    String sKey("hum_alarm_high");
    const FieldDef* pHumAlarm = FieldRegistry::findByKey(sKey);
    TEST_ASSERT_NOT_NULL(pHumAlarm);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MenuField::HumAlarmHigh), static_cast<uint8_t>(pHumAlarm->field));

    // Claves inválidas o vacías (ej. WebEnabled no tiene key MQTT/JSON)
    TEST_ASSERT_NULL(FieldRegistry::findByKey(""));
    TEST_ASSERT_NULL(FieldRegistry::findByKey("clave_inexistente"));
    TEST_ASSERT_NULL(FieldRegistry::findByKey(static_cast<const char*>(nullptr)));
}

void test_field_registry_clamp_value(void) {
    const FieldDef* defSetpoint = FieldRegistry::get(MenuField::Setpoint);
    TEST_ASSERT_NOT_NULL(defSetpoint);
    // Min 20.0, Max 60.0
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 37.5f, FieldRegistry::clampValue(*defSetpoint, 37.5f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 20.0f, FieldRegistry::clampValue(*defSetpoint, 10.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 60.0f, FieldRegistry::clampValue(*defSetpoint, 80.0f));

    // Campo entero (TurnInterval: 1 a 1440 min)
    const FieldDef* defTurn = FieldRegistry::get(MenuField::TurnInterval);
    TEST_ASSERT_NOT_NULL(defTurn);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 240.0f, FieldRegistry::clampValue(*defTurn, 240.4f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 241.0f, FieldRegistry::clampValue(*defTurn, 240.6f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, FieldRegistry::clampValue(*defTurn, 0.1f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1440.0f, FieldRegistry::clampValue(*defTurn, 5000.0f));

    // Campo enum (ControllerType: 0 a 2)
    const FieldDef* defCtl = FieldRegistry::get(MenuField::ControllerType);
    TEST_ASSERT_NOT_NULL(defCtl);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, FieldRegistry::clampValue(*defCtl, 0.2f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, FieldRegistry::clampValue(*defCtl, 1.1f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 2.0f, FieldRegistry::clampValue(*defCtl, 1.9f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, FieldRegistry::clampValue(*defCtl, -5.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 2.0f, FieldRegistry::clampValue(*defCtl, 10.0f));
}

void test_field_registry_get_set(void) {
    SystemConfig cfg;

    const FieldDef* defSetpoint = FieldRegistry::get(MenuField::Setpoint);
    defSetpoint->set(cfg, 38.3f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 38.3f, defSetpoint->get(cfg));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 38.3f, cfg.setpoint);

    const FieldDef* defKp = FieldRegistry::get(MenuField::Kp);
    defKp->set(cfg, 55.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 55.5f, defKp->get(cfg));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 55.5f, cfg.kp);

    const FieldDef* defTurn = FieldRegistry::get(MenuField::TurnDuration);
    defTurn->set(cfg, 15.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 15.0f, defTurn->get(cfg));
    TEST_ASSERT_EQUAL_UINT32(15, cfg.turnDuration);

    const FieldDef* defBuzzer = FieldRegistry::get(MenuField::BuzzerEnabled);
    defBuzzer->set(cfg, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, defBuzzer->get(cfg));
    TEST_ASSERT_FALSE(cfg.buzzerEnabled);

    defBuzzer->set(cfg, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, defBuzzer->get(cfg));
    TEST_ASSERT_TRUE(cfg.buzzerEnabled);
}

void test_field_registry_clamp_cross_dependencies(void) {
    SystemConfig cfg;
    cfg.setpoint = 37.5f;

    // Caso 1: Configuración válida y coherente no se altera
    cfg.tempAlarmHigh  = 38.5f;
    cfg.tempAlarmLow   = 36.5f;
    cfg.humSetpointOn  = 55.0f;
    cfg.humSetpointOff = 65.0f;
    FieldRegistry::clampCrossDependencies(cfg);

    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 38.5f, cfg.tempAlarmHigh);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 36.5f, cfg.tempAlarmLow);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 65.0f, cfg.humSetpointOff);

    // Caso 2: Alarma alta por debajo del setpoint se acota a setpoint
    cfg.tempAlarmHigh = 35.0f;
    FieldRegistry::clampCrossDependencies(cfg);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 37.5f, cfg.tempAlarmHigh);

    // Caso 3: Alarma baja por encima del setpoint se acota a setpoint
    cfg.tempAlarmLow = 39.0f;
    FieldRegistry::clampCrossDependencies(cfg);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 37.5f, cfg.tempAlarmLow);

    // Caso 4: Humedad OFF por debajo de ON se acota a humSetpointOn
    cfg.humSetpointOn = 70.0f;
    cfg.humSetpointOff = 50.0f;
    FieldRegistry::clampCrossDependencies(cfg);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 70.0f, cfg.humSetpointOff);

    // Caso 5: Límites absolutos superiores
    cfg.tempAlarmHigh = 80.0f;
    cfg.humSetpointOff = 120.0f;
    FieldRegistry::clampCrossDependencies(cfg);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 60.0f, cfg.tempAlarmHigh);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 100.0f, cfg.humSetpointOff);
}

void test_field_registry_format_value(void) {
    char buf[32];

    // Formato float con unidad
    const FieldDef* defSetpoint = FieldRegistry::get(MenuField::Setpoint);
    FieldRegistry::formatValue(*defSetpoint, 37.5f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("37.5 C", buf);

    const FieldDef* defHum = FieldRegistry::get(MenuField::HumOn);
    FieldRegistry::formatValue(*defHum, 55.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("55.0 %", buf);

    // Formato entero con unidad
    const FieldDef* defTurn = FieldRegistry::get(MenuField::TurnInterval);
    FieldRegistry::formatValue(*defTurn, 240.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("240 min", buf);

    // Formato enums con etiquetas
    const FieldDef* defCtl = FieldRegistry::get(MenuField::ControllerType);
    FieldRegistry::formatValue(*defCtl, 0.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Histeresis", buf);
    FieldRegistry::formatValue(*defCtl, 1.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("PID", buf);
    FieldRegistry::formatValue(*defCtl, 2.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("LADRC", buf);

    const FieldDef* defBuzzer = FieldRegistry::get(MenuField::BuzzerEnabled);
    FieldRegistry::formatValue(*defBuzzer, 0.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("OFF", buf);
    FieldRegistry::formatValue(*defBuzzer, 1.0f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("ON", buf);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_field_registry_count_and_all);
    RUN_TEST(test_field_registry_get_by_enum);
    RUN_TEST(test_field_registry_find_by_key);
    RUN_TEST(test_field_registry_clamp_value);
    RUN_TEST(test_field_registry_get_set);
    RUN_TEST(test_field_registry_clamp_cross_dependencies);
    RUN_TEST(test_field_registry_format_value);

    return UNITY_END();
}
