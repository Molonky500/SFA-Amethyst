#include "main.h"

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
