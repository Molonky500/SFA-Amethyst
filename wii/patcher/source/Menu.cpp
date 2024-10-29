#include "main.h"

std::shared_ptr<GX::Texture> UI::Menu::texButton;

UI::Menu::Menu() {
    this->posX = 20;
    this->posY = 40;
    this->iSelected = 0;
    this->iFirstDisplayed = 0;
    if(!texButton) {
        texButton = gApp->loadTexture("button");
    }
}

void UI::Menu::handlePointer(int x, int y) {
    int iItem=0;
    for(MenuItem *item : this->items) {
        if(item->enabled && item->hitTest(x, y)) {
            this->iSelected = iItem;
            break;
        }
        iItem++;
    }
}
