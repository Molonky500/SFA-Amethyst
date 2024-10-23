#include "MenuItem.h"

void UI::MenuItem::draw(bool selected) {
    //XXX draw a border of some sort
    fontSetPos(this->posX, this->posY);
    if(this->enabled) {
        if(selected) {
            fontSetColor(hsv2rgb(gFrameCount, 255, 255, 255));
        }
        else fontSetColor({0xCC, 0xCC, 0xCC, 0xFF});
    }
    else fontSetColor({0x44, 0x44, 0x44, 0xFF});
    fontDrawString(this->text.c_str());
}

void UI::MenuItem::measure(int *outX, int *outY) {
    fontMeasureString(this->text.c_str(), outX, outY);
    //XXX padding, border etc
}
