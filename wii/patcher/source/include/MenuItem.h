#pragma once
#include <string>
#include "main.h"
#include "Sprite.h"
extern u32 gFrameCount;

namespace UI {
    class Menu;
    class MenuItem;

    typedef void (*MenuItemActivateFunc)(MenuItem*);

    /**
     * @brief Base class for menu items.
     */
    class MenuItem: public GX::Sprite {
        public:
            MenuItem(const char *text, MenuItemActivateFunc activate);
            virtual ~MenuItem() {
            }

            virtual void draw(bool selected);
            virtual void activate() {
                if(this->activateFunc) (*this->activateFunc)(this);
            }
            virtual MenuItem* setEnabled(bool enabled) {
                this->enabled = enabled;
                return this;
            }

        protected:
            friend class Menu;
            std::string text;
            bool enabled;
            MenuItemActivateFunc activateFunc;
    };
};
