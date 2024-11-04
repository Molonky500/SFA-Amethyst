#include "main.h"
#include "MenuMain.h"
#include "MenuStartGame.h"
#include "MenuManageInstall.h"

static void onStartGame(UI::MenuItem *item, void *param) {
    gApp->enterMenu(new UI::MenuStartGame());
}

static void onManageInstall(UI::MenuItem *item, void *param) {
    gApp->enterMenu(new UI::MenuManageInstall());
}

UI::MenuMain::MenuMain(): UI::MenuMain::Menu() {
    this->addItem(new UI::MenuItem("Start Game", onStartGame));
    this->addItem(new UI::MenuItem("Manage Mods", nullptr));
    this->addItem(new UI::MenuItem("Manage Profiles", nullptr));
    this->addItem(new UI::MenuItem("Settings", nullptr));
    this->addItem(new UI::MenuItem("Manage Installation", onManageInstall));
}
