#include "main.h"

UI::PointerCursor::PointerCursor():
GX::Sprite(nullptr) {
    this->texBg = gApp->loadTexture("shadow_point");
    this->texFg = gApp->loadTexture("generic_point");
    this->texFg->getSize(&this->sizeX, &this->sizeY);
}
