#pragma once
#include "main.h"
#include "Sprite.h"

namespace UI {
    /**
     * @brief Cursor sprite for Wiimote pointer.
     */
    class PointerCursor: public GX::Sprite {
        public:
            PointerCursor():
            Sprite(nullptr) {
                this->texBg = new GX::Texture(
                    gRootDir / "res/shadow_point.tex");
                this->texFg = new GX::Texture(
                    gRootDir / "res/generic_point.tex");
                this->texFg->getSize(&this->sizeX, &this->sizeY);
            }

            virtual ~PointerCursor() {
                if(this->texBg) delete this->texBg;
                if(this->texFg) delete this->texFg;
            }

            virtual void draw() {
                //cursor is offset so that the actual "pointer" part
                //is in the middle.
                this->_drawTexture(this->texBg, 50, 50);
                this->_drawTexture(this->texFg, 48, 48);
            }

        protected:
            GX::Texture *texBg;
            GX::Texture *texFg;
    };
};
