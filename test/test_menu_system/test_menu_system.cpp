#include <unity.h>
#include "display/MenuSystem.h"

void setUp(void) {}
void tearDown(void) {}

void test_menu_initial_state(void) {
    MenuSystem menu;
    menu.begin();

    TEST_ASSERT_EQUAL(MenuPage::Main, menu.currentPage());
    TEST_ASSERT_FALSE(menu.isEditing());
    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(0, menu.scrollOffset());
    TEST_ASSERT_FALSE(menu.confirmChoice());
    TEST_ASSERT_EQUAL(InfoId::None, menu.infoId());
    TEST_ASSERT_EQUAL(MenuAction::None, menu.pendingAction());
}

void test_menu_open_and_root_structure(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    TEST_ASSERT_EQUAL(MenuPage::Menu, menu.currentPage());
    TEST_ASSERT_EQUAL_STRING("Menu", menu.currentTitle());
    TEST_ASSERT_EQUAL_UINT8(6, menu.currentItemCount());

    TEST_ASSERT_EQUAL_STRING("Control", menu.itemLabel(0));
    TEST_ASSERT_EQUAL_STRING("Temperatura", menu.itemLabel(1));
    TEST_ASSERT_EQUAL_STRING("Humedad", menu.itemLabel(2));
    TEST_ASSERT_EQUAL_STRING("Volteo", menu.itemLabel(3));
    TEST_ASSERT_EQUAL_STRING("Sistema", menu.itemLabel(4));
    TEST_ASSERT_EQUAL_STRING("Salir", menu.itemLabel(5));
    TEST_ASSERT_EQUAL_STRING("", menu.itemLabel(6)); // Fuera de rango
}

void test_menu_navigate_and_wrap(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu(); // 6 items: 0..5

    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());

    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(1, menu.selectedItem());

    menu.navigate(3);
    TEST_ASSERT_EQUAL_UINT8(4, menu.selectedItem());

    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(5, menu.selectedItem());

    // Wrap forward
    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());

    // Wrap backward
    menu.navigate(-1);
    TEST_ASSERT_EQUAL_UINT8(5, menu.selectedItem());
}

void test_menu_scroll_behavior(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    // Entrar a Control (10 items: 0..9)
    menu.confirm(); // Item 0 es Control
    TEST_ASSERT_EQUAL_STRING("Control", menu.currentTitle());
    TEST_ASSERT_EQUAL_UINT8(10, menu.currentItemCount());
    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(0, menu.scrollOffset());

    // Navegar hasta el item 7 (visible sin scroll, VISIBLE_ROWS = 8)
    menu.navigate(7);
    TEST_ASSERT_EQUAL_UINT8(7, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(0, menu.scrollOffset());

    // Navegar al item 8 -> scroll debe desplazarse a 1
    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(8, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(1, menu.scrollOffset());

    // Navegar al item 9 -> scroll a 2
    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(9, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(2, menu.scrollOffset());

    // Volver hacia atrás a item 8 -> scroll se mantiene en 2 porque 8 es visible en [2..9]
    menu.navigate(-1);
    TEST_ASSERT_EQUAL_UINT8(8, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(2, menu.scrollOffset());

    // Navegar hacia arriba hasta que el scroll deba reducirse
    menu.navigate(-7); // item 1
    TEST_ASSERT_EQUAL_UINT8(1, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(1, menu.scrollOffset());

    menu.navigate(-1); // item 0
    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());
    TEST_ASSERT_EQUAL_UINT8(0, menu.scrollOffset());
}

void test_menu_enter_submenu_and_back_restores_state(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    // Navegar a "Temperatura" (índice 1 en Root)
    menu.navigate(1);
    TEST_ASSERT_EQUAL_UINT8(1, menu.selectedItem());

    // Entrar a Temperatura
    menu.confirm();
    TEST_ASSERT_EQUAL_STRING("Temperatura", menu.currentTitle());
    TEST_ASSERT_EQUAL_UINT8(0, menu.selectedItem());

    // Moverse adentro
    menu.navigate(2); // "Alarma T Baja"
    TEST_ASSERT_EQUAL_UINT8(2, menu.selectedItem());

    // Volver con cancel()
    menu.cancel();
    TEST_ASSERT_EQUAL_STRING("Menu", menu.currentTitle());
    TEST_ASSERT_EQUAL_UINT8(1, menu.selectedItem()); // Restaura posición previa
    TEST_ASSERT_EQUAL_UINT8(0, menu.scrollOffset());

    // Salir del menú con cancel() en root
    menu.cancel();
    TEST_ASSERT_EQUAL(MenuPage::Main, menu.currentPage());
}

void test_menu_edit_lifecycle_and_callbacks(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    MenuField enteredField = MenuField::Count;
    MenuField exitedField  = MenuField::Count;
    int valueDeltas = 0;

    menu.setOnEnterEdit([&](MenuField f) { enteredField = f; });
    menu.setOnExitEdit([&](MenuField f)  { exitedField  = f; });
    menu.setOnValueChange([&](int d)     { valueDeltas += d;  });

    // Entrar a Control
    menu.confirm();
    // Item 0 es Setpoint (Kind::Field)
    menu.confirm();

    TEST_ASSERT_TRUE(menu.isEditing());
    TEST_ASSERT_EQUAL(MenuPage::EditValue, menu.currentPage());
    TEST_ASSERT_EQUAL(MenuField::Setpoint, menu.editField());
    TEST_ASSERT_EQUAL(MenuField::Setpoint, enteredField);

    // En edición, navigate llama al callback de delta
    menu.navigate(1);
    TEST_ASSERT_EQUAL(1, valueDeltas);
    menu.navigate(-3);
    TEST_ASSERT_EQUAL(-2, valueDeltas);

    // Salir de edición
    menu.cancel();
    TEST_ASSERT_FALSE(menu.isEditing());
    TEST_ASSERT_EQUAL(MenuPage::Menu, menu.currentPage());
    TEST_ASSERT_EQUAL(MenuField::Setpoint, exitedField);
}

void test_menu_confirm_dialogs_and_actions(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    MenuAction executedAction = MenuAction::None;
    menu.setOnAction([&](MenuAction a) { executedAction = a; });

    // Ir a Sistema (índice 4 en Root)
    menu.navigate(4);
    menu.confirm();
    TEST_ASSERT_EQUAL_STRING("Sistema", menu.currentTitle());

    // En Sistema, item 6 es "Restaurar Fab." (Kind::Confirm, FactoryReset)
    menu.navigate(6);
    menu.confirm();

    TEST_ASSERT_EQUAL(MenuPage::Confirm, menu.currentPage());
    TEST_ASSERT_FALSE(menu.confirmChoice());
    TEST_ASSERT_EQUAL(MenuAction::FactoryReset, menu.pendingAction());

    // Si confirmamos cuando confirmChoice == false, se descarta y no se ejecuta acción
    menu.confirm();
    TEST_ASSERT_EQUAL(MenuPage::Menu, menu.currentPage());
    TEST_ASSERT_EQUAL(MenuAction::None, executedAction);

    // Entrar de nuevo al diálogo
    menu.confirm();
    TEST_ASSERT_EQUAL(MenuPage::Confirm, menu.currentPage());

    // Alternar selección a TRUE con navigate
    menu.navigate(1);
    TEST_ASSERT_TRUE(menu.confirmChoice());

    // Ahora confirmar ejecuta la acción
    menu.confirm();
    TEST_ASSERT_EQUAL(MenuPage::Menu, menu.currentPage());
    TEST_ASSERT_EQUAL(MenuAction::FactoryReset, executedAction);
}

void test_menu_info_dialogs(void) {
    MenuSystem menu;
    menu.begin();
    menu.openMenu();

    // Ir a Sistema (índice 4)
    menu.navigate(4);
    menu.confirm();

    // Item 1 es "Ver IP / Modo" (Kind::Info, InfoId::Net)
    menu.navigate(1);
    menu.confirm();

    TEST_ASSERT_EQUAL(MenuPage::Info, menu.currentPage());
    TEST_ASSERT_EQUAL(InfoId::Net, menu.infoId());

    // Salir con cancel
    menu.cancel();
    TEST_ASSERT_EQUAL(MenuPage::Menu, menu.currentPage());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_menu_initial_state);
    RUN_TEST(test_menu_open_and_root_structure);
    RUN_TEST(test_menu_navigate_and_wrap);
    RUN_TEST(test_menu_scroll_behavior);
    RUN_TEST(test_menu_enter_submenu_and_back_restores_state);
    RUN_TEST(test_menu_edit_lifecycle_and_callbacks);
    RUN_TEST(test_menu_confirm_dialogs_and_actions);
    RUN_TEST(test_menu_info_dialogs);
    return UNITY_END();
}
