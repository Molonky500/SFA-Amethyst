#pragma once
#include "Menu.h"

namespace UI {
    class MenuInstallFromDisc: public Menu {
        public:
            MenuInstallFromDisc();
            ~MenuInstallFromDisc() {
                if(this->sprStatusBg) delete this->sprStatusBg;
            }

            virtual void draw();

        protected:
            GX::Sprite *sprStatusBg;
            bool isDiscReady;
            UI::MenuItem *itemInstall;
    };
};
