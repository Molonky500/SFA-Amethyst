#include "main.h"
#include "MenuManageInstall.h"

static void onInstallFromDisc(UI::MenuItem *item) {
    //this should be a submenu that shows the status and so on
    Sys::DiscDrive dvd;
    u64 discId = 0;
    int err;
    do {
        sleep(1);
        err = dvd.getDiscId(discId);
    } while(err == -EAGAIN);
    printf("Disc ID = 0x%llX err %d\r\n", discId, err);
}

static void onBack(UI::MenuItem *item) {
    gApp->exitMenu();
}

UI::MenuManageInstall::MenuManageInstall(): UI::MenuManageInstall::Menu() {
    this->addItem(new UI::MenuItem("Install from Disc", onInstallFromDisc));
    this->addItem(new UI::MenuItem("Back", onBack));
}
