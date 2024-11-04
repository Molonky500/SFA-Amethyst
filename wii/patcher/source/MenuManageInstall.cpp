#include "main.h"
#include "MenuManageInstall.h"
#include "MenuInstallFromDisc.h"

static void onInstallFromDisc(UI::MenuItem *item, void *param) {
    gApp->enterMenu(new UI::MenuInstallFromDisc());
}

static void onBack(UI::MenuItem *item, void *param) {
    gApp->exitMenu();
}

UI::MenuManageInstall::MenuManageInstall(): UI::MenuManageInstall::Menu() {
    this->addItem(new UI::MenuItem("Install from Disc", onInstallFromDisc));
    this->addItem(new UI::MenuItem("Back", onBack));
}
