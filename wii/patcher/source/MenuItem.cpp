#include "main.h"

UI::MenuItem::MenuItem(const char *text, MenuItemActivateFunc activate,
void *activateParam):
GX::Sprite(nullptr) {
    this->text = text;
    this->enabled = true;
    this->activateFunc = activate;
    this->activateParam = activateParam;
    this->setTexture(UI::Menu::texButton);
}

void UI::MenuItem::draw(bool selected) {
    float alpha = 1.0f;
    if(!this->enabled) alpha = 0.25f;
    else if(!selected) alpha = 0.5f;
    this->setOpacity(alpha);
    GX::Sprite::draw();

    auto font = gApp->getSystemFont();
    font->setSize(30);
    int offsX =  8; //XXX hardcoded offset
    int offsY = 44;

    //drop shadow
    font->setPos(this->posX + offsX + 2, this->posY + offsY + 2)
        ->setColor({0x00, 0x00, 0x00, 0x80})
        ->drawString(this->text.c_str());

    //text
    GX::Color color = {0x44, 0x44, 0x44, 0xFF};
    if(this->enabled) {
        if(selected) color = GX::hsv2rgb(
            gApp->getFrameCount(), 255, 255, 255);
        else color = {0xCC, 0xCC, 0xCC, 0xFF};
    }
    font->setColor(color)
        ->setPos(this->posX + offsX, this->posY + offsY)
        ->drawString(this->text.c_str());
}
