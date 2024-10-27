#pragma once
#include "main.h"
#include "Sprite.h"

namespace UI {
    /**
     * @brief Cursor sprite for Wiimote pointer.
     */
    class PointerCursor: public GX::Sprite {
        public:
            PointerCursor();

            virtual ~PointerCursor() {
            }

            virtual void draw() {
                //cursor is offset so that the actual "pointer" part
                //is in the middle.
                this->_drawTexture(&*this->texBg, -48, -48);
                this->_drawTexture(&*this->texFg, -50, -50);
            }

        protected:
            std::shared_ptr<GX::Texture> texBg;
            std::shared_ptr<GX::Texture> texFg;
    };
};
