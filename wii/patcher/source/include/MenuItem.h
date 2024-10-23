#pragma once
#include <string>
#include "main.h"
extern u32 gFrameCount;

namespace UI {
    class Menu;
    class MenuItem;

    typedef void (*MenuItemActivateFunc)(MenuItem*);

    /**
     * @brief Base class for menu items.
     */
    class MenuItem {
        public:
            MenuItem(const char *text, MenuItemActivateFunc activate) {
                this->posX = 0.0f;
                this->posY = 0.0f;
                this->text = text;
                this->enabled = true;
                this->activateFunc = activate;
            }

            virtual ~MenuItem() {
            }

            virtual void draw(bool selected);
            virtual void measure(int *outX, int *outY);
            virtual void activate() {
                if(this->activateFunc) (*this->activateFunc)(this);
            }

        protected:
            friend class Menu;
            float posX, posY, sizeX, sizeY;
            std::string text;
            bool enabled;
            MenuItemActivateFunc activateFunc;
    };
};
