#include <unity.h>
#include "alarm/AlarmManager.h"
#include <string>

static Buzzer g_buzzer;
static OutputState g_outputs;
static AlarmManager g_alarmMgr;
static SystemConfig g_config;

void setUp(void) {
    mock_set_millis(1000);
    getMockBuzzer().reset();
    g_outputs = OutputState{};
    g_alarmMgr.begin(&g_buzzer, &g_outputs);

    // Configuración base de umbrales
    g_config = SystemConfig{};
    g_config.setpoint = 37.5f;
    g_config.tempAlarmHigh = 38.0f;
    g_config.tempAlarmLow  = 37.0f;
    g_config.humAlarmHigh  = 75.0f;
    g_config.humAlarmLow   = 45.0f;
}

void tearDown(void) {}

void test_alarm_manager_init(void) {
    TEST_ASSERT_FALSE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(0, g_alarmMgr.mask());
    TEST_ASSERT_EQUAL_STRING("", g_alarmMgr.message());
    TEST_ASSERT_FALSE(g_outputs.buzzerActive);
    TEST_ASSERT_EQUAL(0, getMockBuzzer().playCount);
}

void test_alarm_manager_invalid_sensor_no_alarm(void) {
    SensorData sensor = {45.0f, 95.0f, false}; // Sensor con lectura no válida
    g_alarmMgr.evaluate(sensor, g_config);

    TEST_ASSERT_FALSE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(0, g_alarmMgr.mask());
    TEST_ASSERT_FALSE(g_outputs.buzzerActive);
    TEST_ASSERT_EQUAL(0, getMockBuzzer().playCount);
}

void test_alarm_manager_temp_high_trigger(void) {
    SensorData sensor = {38.5f, 55.0f, true};
    g_alarmMgr.evaluate(sensor, g_config);

    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(AlarmFlag::AlarmTempHigh, g_alarmMgr.mask());
    TEST_ASSERT_TRUE(g_outputs.buzzerActive);
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Temp ALTA") != nullptr);
    TEST_ASSERT_EQUAL(1, getMockBuzzer().playCount);
}

void test_alarm_manager_temp_low_trigger(void) {
    SensorData sensor = {36.8f, 55.0f, true};
    g_alarmMgr.evaluate(sensor, g_config);

    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(AlarmFlag::AlarmTempLow, g_alarmMgr.mask());
    TEST_ASSERT_TRUE(g_outputs.buzzerActive);
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Temp BAJA") != nullptr);
    TEST_ASSERT_EQUAL(1, getMockBuzzer().playCount);
}

void test_alarm_manager_hum_alarms(void) {
    // Humedad Alta
    SensorData sensorHigh = {37.5f, 80.0f, true};
    g_alarmMgr.evaluate(sensorHigh, g_config);

    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(AlarmFlag::AlarmHumHigh, g_alarmMgr.mask());
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Hum ALTA") != nullptr);

    // Humedad Baja
    SensorData sensorLow = {37.5f, 40.0f, true};
    g_alarmMgr.evaluate(sensorLow, g_config);

    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(AlarmFlag::AlarmHumLow, g_alarmMgr.mask());
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Hum BAJA") != nullptr);
}

void test_alarm_manager_priority_hierarchy(void) {
    // Caso 1: Temp ALTA y Hum BAJA simultáneas (Temp ALTA debe ganar en melodía)
    SensorData sensorCombo1 = {39.0f, 35.0f, true};
    g_alarmMgr.evaluate(sensorCombo1, g_config);

    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    uint8_t expectedMask1 = AlarmFlag::AlarmTempHigh | AlarmFlag::AlarmHumLow;
    TEST_ASSERT_EQUAL_UINT8(expectedMask1, g_alarmMgr.mask());
    // Ambas deben reportarse en el mensaje
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Temp ALTA") != nullptr);
    TEST_ASSERT_TRUE(strstr(g_alarmMgr.message(), "Hum BAJA") != nullptr);
    // Pero la melodía asignada es la de Temp ALTA (t_size = 2)
    TEST_ASSERT_EQUAL_UINT8(2, getMockBuzzer().lastBuffer.t_size);

    // Caso 2: Temp BAJA y Hum ALTA simultáneas (Temp BAJA gana a Hum ALTA)
    SensorData sensorCombo2 = {36.0f, 85.0f, true};
    g_alarmMgr.evaluate(sensorCombo2, g_config);
    uint8_t expectedMask2 = AlarmFlag::AlarmTempLow | AlarmFlag::AlarmHumHigh;
    TEST_ASSERT_EQUAL_UINT8(expectedMask2, g_alarmMgr.mask());
    // Melodía asignada es Temp BAJA (t_size = 2)
    TEST_ASSERT_EQUAL_UINT8(2, getMockBuzzer().lastBuffer.t_size);

    // Caso 3: Solo Hum ALTA (t_size = 3)
    SensorData sensorHumHigh = {37.5f, 85.0f, true};
    g_alarmMgr.evaluate(sensorHumHigh, g_config);
    TEST_ASSERT_EQUAL_UINT8(AlarmFlag::AlarmHumHigh, g_alarmMgr.mask());
    TEST_ASSERT_EQUAL_UINT8(3, getMockBuzzer().lastBuffer.t_size);
}

void test_alarm_manager_clear_and_silence(void) {
    SensorData badSensor = {39.0f, 50.0f, true};
    g_alarmMgr.evaluate(badSensor, g_config);
    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_TRUE(g_outputs.buzzerActive);

    // Sensor se normaliza
    SensorData normalSensor = {37.5f, 50.0f, true};
    g_alarmMgr.evaluate(normalSensor, g_config);

    TEST_ASSERT_FALSE(g_alarmMgr.isActive());
    TEST_ASSERT_EQUAL_UINT8(0, g_alarmMgr.mask());
    TEST_ASSERT_FALSE(g_outputs.buzzerActive);
    TEST_ASSERT_TRUE(getMockBuzzer().stopCount > 0);
}

void test_alarm_manager_snooze(void) {
    SensorData badSensor = {39.0f, 50.0f, true};
    g_alarmMgr.evaluate(badSensor, g_config);
    TEST_ASSERT_TRUE(g_alarmMgr.isActive());

    // Snooze de 600000 ms (10 min)
    int stopsBefore = getMockBuzzer().stopCount;
    g_alarmMgr.snooze(600000);

    TEST_ASSERT_FALSE(g_alarmMgr.isActive());
    TEST_ASSERT_FALSE(g_outputs.buzzerActive);
    TEST_ASSERT_TRUE(getMockBuzzer().stopCount > stopsBefore);

    // Mientras dure el snooze, evaluate no reactiva la alarma
    mock_advance_millis(300000); // pasaron 5 min
    g_alarmMgr.evaluate(badSensor, g_config);
    TEST_ASSERT_FALSE(g_alarmMgr.isActive());
    TEST_ASSERT_FALSE(g_outputs.buzzerActive);

    // Al expirar el snooze (+300001 ms, total 600001 ms)
    mock_advance_millis(300001);
    g_alarmMgr.evaluate(badSensor, g_config);
    TEST_ASSERT_TRUE(g_alarmMgr.isActive());
    TEST_ASSERT_TRUE(g_outputs.buzzerActive);
}

void test_alarm_manager_publish_hook(void) {
    std::string topicReceived;
    std::string payloadReceived;

    g_alarmMgr.setPublishHook([&](const char* topic, const char* payload) {
        topicReceived = topic;
        payloadReceived = payload;
    });

    SensorData badSensor = {39.2f, 50.0f, true};
    g_alarmMgr.evaluate(badSensor, g_config);

    TEST_ASSERT_EQUAL_STRING("incubadora/status/alarma", topicReceived.c_str());
    TEST_ASSERT_TRUE(payloadReceived.find("Temp ALTA") != std::string::npos);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_alarm_manager_init);
    RUN_TEST(test_alarm_manager_invalid_sensor_no_alarm);
    RUN_TEST(test_alarm_manager_temp_high_trigger);
    RUN_TEST(test_alarm_manager_temp_low_trigger);
    RUN_TEST(test_alarm_manager_hum_alarms);
    RUN_TEST(test_alarm_manager_priority_hierarchy);
    RUN_TEST(test_alarm_manager_clear_and_silence);
    RUN_TEST(test_alarm_manager_snooze);
    RUN_TEST(test_alarm_manager_publish_hook);
    return UNITY_END();
}
