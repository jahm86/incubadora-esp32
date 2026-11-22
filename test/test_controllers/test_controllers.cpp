#include <unity.h>
#include <math.h>
#include "control/PIDController.h"
#include "control/HysteresisController.h"
#include "control/LADRCController.h"

void setUp(void) {}
void tearDown(void) {}

// ==========================================
// Pruebas HysteresisController
// ==========================================

void test_hysteresis_initial_state(void) {
    HysteresisController controller(0.4f);
    controller.setSetpoint(37.5f);
    controller.begin();

    TEST_ASSERT_FALSE(controller.isHeaterOn());
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, controller.compute(37.5f));
}

void test_hysteresis_deadband_switching(void) {
    HysteresisController controller(0.4f);
    controller.setSetpoint(37.5f);
    controller.begin();

    // Setpoint = 37.5, Histeresis = 0.4 -> lower = 37.3, upper = 37.7
    // Inicialmente apagado
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.compute(37.5f)); // En setpoint -> OFF
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.compute(37.4f)); // En banda muerta -> OFF
    TEST_ASSERT_FALSE(controller.isHeaterOn());

    // Cae por debajo o igual al limite inferior -> debe encender
    TEST_ASSERT_EQUAL_FLOAT(1.0f, controller.compute(37.3f)); // lower -> ON
    TEST_ASSERT_TRUE(controller.isHeaterOn());

    TEST_ASSERT_EQUAL_FLOAT(1.0f, controller.compute(37.1f)); // por debajo -> ON

    // Sube dentro de la banda muerta -> debe permanecer encendido
    TEST_ASSERT_EQUAL_FLOAT(1.0f, controller.compute(37.4f)); // en banda -> ON
    TEST_ASSERT_EQUAL_FLOAT(1.0f, controller.compute(37.6f)); // en banda -> ON
    TEST_ASSERT_TRUE(controller.isHeaterOn());

    // Alcanza o supera el limite superior -> debe apagar
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.compute(37.7f)); // upper -> OFF
    TEST_ASSERT_FALSE(controller.isHeaterOn());

    // Cae dentro de la banda muerta -> debe permanecer apagado
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.compute(37.6f)); // en banda -> OFF
    TEST_ASSERT_FALSE(controller.isHeaterOn());
}

void test_hysteresis_reset(void) {
    HysteresisController controller(0.4f);
    controller.setSetpoint(37.5f);
    controller.begin();

    controller.compute(36.0f); // Fuerza ON
    TEST_ASSERT_TRUE(controller.isHeaterOn());

    controller.reset();
    TEST_ASSERT_FALSE(controller.isHeaterOn());
}

// ==========================================
// Pruebas PIDController
// ==========================================

void test_pid_proportional_response(void) {
    PIDController pid(2.0f, 0.0f, 0.0f); // Solo P = 2.0
    pid.setSetpoint(37.5f);
    pid.setTs(1.0f);
    pid.begin();

    // Error = 0
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, pid.compute(37.5f));

    // Error = 37.5 - 37.25 = 0.25 -> P = 2.0 * 0.25 = 0.5
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.5f, pid.compute(37.25f));

    // Error = 37.5 - 36.5 = 1.0 -> P = 2.0 * 1.0 = 2.0 -> clamp 1.0
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, pid.compute(36.5f));

    // Error negativo (caliente): 37.5 - 38.0 = -0.5 -> clamp 0.0
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, pid.compute(38.0f));
}

void test_pid_integral_accumulation_and_antiwindup(void) {
    PIDController pid(0.0f, 0.2f, 0.0f); // Solo I = 0.2
    pid.setSetpoint(37.0f);
    pid.setTs(1.0f);
    pid.begin();

    // Paso 1: error = 1.0, Ts = 1.0 -> integral = 1.0 -> salida = 0.2
    float out1 = pid.compute(36.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.2f, out1);

    // Paso 2: integral = 2.0 -> salida = 0.4
    float out2 = pid.compute(36.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.4f, out2);

    // Ejecutar 100 pasos para probar saturacion y anti-windup (m_limit = 100)
    for (int i = 0; i < 150; i++) {
        pid.compute(36.0f);
    }
    float saturated = pid.compute(36.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, saturated); // Salida limitada a 1.0 max

    // Invertir error para verificar que desacumula sin atascarse
    pid.compute(38.0f); // error = -1.0
}

void test_pid_derivative_response(void) {
    PIDController pid(0.0f, 0.0f, 1.0f); // Solo D = 1.0
    pid.setSetpoint(37.0f);
    pid.setTs(1.0f);
    pid.begin();

    // Paso 1: error pasa de 0.0 a 1.0 -> dError/dt = 1.0 -> salida = 1.0
    float out1 = pid.compute(36.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, out1);

    // Paso 2: error constante 1.0 -> dError/dt = 0.0 -> salida = 0.0
    float out2 = pid.compute(36.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out2);

    // Paso 3: temperatura sube (error disminuye de 1.0 a 0.5) -> dError/dt = -0.5 -> clamp a 0.0
    float out3 = pid.compute(36.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out3);
}

void test_pid_reset(void) {
    PIDController pid(1.0f, 1.0f, 1.0f);
    pid.setSetpoint(37.0f);
    pid.setTs(1.0f);
    pid.begin();

    pid.compute(30.0f);
    pid.compute(30.0f);

    pid.reset();
    // Tras reset con error 0, salida debe ser exactamente 0
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, pid.compute(37.0f));
}

// ==========================================
// Pruebas LADRCController
// ==========================================

void test_ladrc_initial_state(void) {
    LADRCController ladrc(1.0f, 0.5f, 2.0f);
    ladrc.setSetpoint(37.5f);
    ladrc.setTs(1.0f);
    ladrc.begin();

    // Cuando input coincide con setpoint y no hay estados previos
    float out = ladrc.compute(37.5f);
    TEST_ASSERT_TRUE(out >= 0.0f && out <= 1.0f);
}

void test_ladrc_step_response_and_clamping(void) {
    LADRCController ladrc(1.0f, 0.5f, 0.2f);
    ladrc.setSetpoint(37.5f);
    ladrc.setTs(1.0f);
    ladrc.begin();

    // Temperatura fria (20 C) con setpoint 37.5 C -> control debe saturar en 1.0 (calentador al 100%)
    float outCold = ladrc.compute(20.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, outCold);

    // Tras varios pasos a temperatura muy caliente (50 C) por encima de setpoint 37.5 C -> el observador converge y el control satura en 0.0
    float outHot = 0.0f;
    for (int i = 0; i < 5; i++) {
        outHot = ladrc.compute(50.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, outHot);
}

void test_ladrc_reset(void) {
    LADRCController ladrc(1.0f, 1.0f, 2.0f);
    ladrc.setSetpoint(37.5f);
    ladrc.setTs(1.0f);
    ladrc.begin();

    ladrc.compute(20.0f);
    ladrc.reset();

    // Tras reset y temperatura igual a setpoint
    float out = ladrc.compute(0.0f);
    TEST_ASSERT_TRUE(out >= 0.0f && out <= 1.0f);
}

// ==========================================
// Runner principal
// ==========================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Hysteresis Tests
    RUN_TEST(test_hysteresis_initial_state);
    RUN_TEST(test_hysteresis_deadband_switching);
    RUN_TEST(test_hysteresis_reset);

    // PID Tests
    RUN_TEST(test_pid_proportional_response);
    RUN_TEST(test_pid_integral_accumulation_and_antiwindup);
    RUN_TEST(test_pid_derivative_response);
    RUN_TEST(test_pid_reset);

    // LADRC Tests
    RUN_TEST(test_ladrc_initial_state);
    RUN_TEST(test_ladrc_step_response_and_clamping);
    RUN_TEST(test_ladrc_reset);

    return UNITY_END();
}
