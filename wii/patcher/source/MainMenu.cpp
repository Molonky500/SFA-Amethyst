#include "main.h"
#include "MainMenu.h"

static void MenuItemOneActivate(UI::MenuItem *item) {
    printf("This is item one\r\n");
}

UI::MainMenu::MainMenu(): UI::MainMenu::Menu() {
    this->addItem(new UI::MenuItem("First Item", MenuItemOneActivate));
    this->addItem(new UI::MenuItem("Item the Second", nullptr));
    this->addItem((new UI::MenuItem("here's three!", nullptr))
        ->setEnabled(false));
    this->addItem(new UI::MenuItem("This item's text is way too long what the hell kind of menu item even is this supposed to be", nullptr));
}
