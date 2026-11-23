#include <unity.h>
#include "output/EggTray.h"
#include "config/pins.h"

void setUp(void) {
    mock_reset_pins();
}

void tearDown(void) {}

void test_egg_tray_begin(void) {
    EggTray tray;
    tray.begin();

    TEST_ASSERT_EQUAL(OUTPUT, mock_pin_mode_ref(PIN_EGG_TRAY));
    TEST_ASSERT_EQUAL(LOW, mock_digital_pin_ref(PIN_EGG_TRAY));
    TEST_ASSERT_FALSE(tray.isActive());
}

void test_egg_tray_on(void) {
    EggTray tray;
    tray.begin();

    tray.on();
    TEST_ASSERT_EQUAL(HIGH, mock_digital_pin_ref(PIN_EGG_TRAY));
    TEST_ASSERT_TRUE(tray.isActive());
}

void test_egg_tray_off(void) {
    EggTray tray;
    tray.begin();

    tray.on();
    TEST_ASSERT_TRUE(tray.isActive());

    tray.off();
    TEST_ASSERT_EQUAL(LOW, mock_digital_pin_ref(PIN_EGG_TRAY));
    TEST_ASSERT_FALSE(tray.isActive());
}

void test_egg_tray_idempotency(void) {
    EggTray tray;
    tray.begin();

    // Múltiples activaciones sucesivas
    tray.on();
    tray.on();
    TEST_ASSERT_EQUAL(HIGH, mock_digital_pin_ref(PIN_EGG_TRAY));
    TEST_ASSERT_TRUE(tray.isActive());

    // Múltiples desactivaciones sucesivas
    tray.off();
    tray.off();
    TEST_ASSERT_EQUAL(LOW, mock_digital_pin_ref(PIN_EGG_TRAY));
    TEST_ASSERT_FALSE(tray.isActive());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_egg_tray_begin);
    RUN_TEST(test_egg_tray_on);
    RUN_TEST(test_egg_tray_off);
    RUN_TEST(test_egg_tray_idempotency);
    return UNITY_END();
}
