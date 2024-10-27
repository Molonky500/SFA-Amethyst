#include "main.h"
#include "MenuStartGame.h"

static void onStartGame(UI::MenuItem *item) {
    printf("GEEMU STAATO\r\n");
}

static void onBack(UI::MenuItem *item) {
    gApp->exitMenu();
}

UI::MenuStartGame::MenuStartGame(): UI::MenuStartGame::Menu() {
    this->addItem(new UI::MenuItem("Start with Some Mod", onStartGame));
    this->addItem(new UI::MenuItem("Start with Another Mod", onStartGame));
    this->addItem(new UI::MenuItem("Back", onBack));
}
