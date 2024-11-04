#pragma once
#include <string>
#include "main.h"
#include "Sprite.h"
extern u32 gFrameCount;

namespace UI {
    class Menu;
    class MenuItem;

    typedef void (*MenuItemActivateFunc)(MenuItem*, void*);

    /**
     * @brief Base class for menu items.
     */
    class MenuItem: public GX::Sprite {
        public:
            MenuItem(const char *text, MenuItemActivateFunc activate,
            void *activateParam=nullptr);
            virtual ~MenuItem() {
            }

            virtual void draw(bool selected);
            virtual void activate() {
                if(this->activateFunc) {
                    (*this->activateFunc)(this, this->activateParam);
                }
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
            void *activateParam;
    };
};
