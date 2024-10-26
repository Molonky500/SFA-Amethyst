#include "MenuItem.h"

void UI::MenuItem::draw(bool selected) {
    float alpha = 1.0f;
    if(!this->enabled) alpha = 0.25f;
    else if(!selected) alpha = 0.5f;

    int offsX=0, offsY=0;
    if(&*this->texBg) {
        offsX =  8; //XXX hardcoded offset
        offsY = 44;
        appDrawSprite(&*this->texBg, this->posX, this->posY, alpha);
    }
    fontSetSize(30);

    //drop shadow
    fontSetPos(this->posX + offsX + 2, this->posY + offsY + 2);
    fontSetColor({0x00, 0x00, 0x00, 0x80});
    fontDrawString(this->text.c_str());

    fontSetPos(this->posX + offsX, this->posY + offsY);
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
    if(&*this->texBg) {
        u16 w, h;
        this->texBg->getSize(&w, &h);
        *outX = w;
        *outY = h;
    }
    else {
        fontSetSize(30);
        fontMeasureString(this->text.c_str(), outX, outY);
        *outX += 2; *outY += 2; //shadow
    }
    *outY += 8; //padding
}
