#include <unity.h>
#include "core/IncubationTracker.h"
#include <esp_timer.h>

void setUp(void) {
    mock_set_esp_timer_time(1000000LL); // 1 segundo inicial
}

void tearDown(void) {}

void test_tracker_begin_zero(void) {
    IncubationTracker tracker;
    tracker.begin(0);

    TEST_ASSERT_EQUAL_UINT32(0, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(0, tracker.days());
}

void test_tracker_begin_with_offset(void) {
    IncubationTracker tracker;
    // Iniciar con 3 días y 120 segundos
    const uint32_t initialSeconds = (3 * 86400) + 120;
    tracker.begin(initialSeconds);

    TEST_ASSERT_EQUAL_UINT32(initialSeconds, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(3, tracker.days());
}

void test_tracker_elapsed_advancement(void) {
    IncubationTracker tracker;
    tracker.begin(0);

    // Avanzar 3600 segundos (1 hora)
    mock_advance_esp_timer_time(3600LL * 1000000LL);
    TEST_ASSERT_EQUAL_UINT32(3600, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(0, tracker.days());

    // Avanzar 82800 segundos más (total 86400 s = exactamente 1 día)
    mock_advance_esp_timer_time(82800LL * 1000000LL);
    TEST_ASSERT_EQUAL_UINT32(86400, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(1, tracker.days());

    // Avanzar medio día más (43200 s)
    mock_advance_esp_timer_time(43200LL * 1000000LL);
    TEST_ASSERT_EQUAL_UINT32(86400 + 43200, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(1, tracker.days());
}

void test_tracker_reset(void) {
    IncubationTracker tracker;
    tracker.begin(86400 * 5); // 5 días previos
    TEST_ASSERT_EQUAL_UINT32(5, tracker.days());

    // Resetear contador
    tracker.reset();
    TEST_ASSERT_EQUAL_UINT32(0, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(0, tracker.days());

    // Continuar avanzando tiempo después del reset
    mock_advance_esp_timer_time(120LL * 1000000LL); // 2 minutos
    TEST_ASSERT_EQUAL_UINT32(120, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(0, tracker.days());
}

void test_tracker_no_overflow_beyond_49_days(void) {
    IncubationTracker tracker;
    tracker.begin(0);

    // En 32 bits con microsegundos (uint32_t micros()), el contador se desborda
    // a los 49.71 días (2^32 us = 4,294,967,296 us).
    // Con base de 64 bits (int64_t esp_timer_get_time()), no se produce desbordamiento.

    // Simulamos un salto temporal de 55 días (> 49.71 días)
    const int64_t fiftyFiveDaysUs = 55LL * 86400LL * 1000000LL;
    mock_advance_esp_timer_time(fiftyFiveDaysUs);

    TEST_ASSERT_EQUAL_UINT32(55 * 86400, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(55, tracker.days());

    // Avanzamos hasta 100 días
    const int64_t fortyFiveDaysMoreUs = 45LL * 86400LL * 1000000LL;
    mock_advance_esp_timer_time(fortyFiveDaysMoreUs);

    TEST_ASSERT_EQUAL_UINT32(100 * 86400, tracker.elapsedSeconds());
    TEST_ASSERT_EQUAL_UINT32(100, tracker.days());
}

void test_tracker_negative_diff_guard(void) {
    IncubationTracker tracker;
    tracker.begin(100);

    // Si por alguna anomalía el temporizador reporta un valor anterior al inicio
    mock_advance_esp_timer_time(-500000LL);

    // Debe protegerse y no producir overflow o números gigantes
    TEST_ASSERT_EQUAL_UINT32(100, tracker.elapsedSeconds());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tracker_begin_zero);
    RUN_TEST(test_tracker_begin_with_offset);
    RUN_TEST(test_tracker_elapsed_advancement);
    RUN_TEST(test_tracker_reset);
    RUN_TEST(test_tracker_no_overflow_beyond_49_days);
    RUN_TEST(test_tracker_negative_diff_guard);
    return UNITY_END();
}
