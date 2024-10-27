#include "main.h"
#include "MainMenu.h"

static void MenuItemOneActivate(UI::MenuItem *item) {
    printf("This is item one\r\n");
}

UI::MainMenu::MainMenu(): UI::MainMenu::Menu() {
    auto *texButton = gApp->loadTexture("button");

    auto item = new UI::MenuItem("First Item", MenuItemOneActivate);
    item->setTexture(texButton);
    this->addItem(item);

    item = new UI::MenuItem("Item the Second", nullptr);
    item->setTexture(texButton);
    this->addItem(item);

    item = new UI::MenuItem("here's three!", nullptr);
    item->setEnabled(false)->setTexture(texButton);
    this->addItem(item);

    item = new UI::MenuItem("This item's text is way too long what the hell kind of menu item even is this supposed to be", nullptr);
    item->setTexture(texButton);
    this->addItem(item);
}
