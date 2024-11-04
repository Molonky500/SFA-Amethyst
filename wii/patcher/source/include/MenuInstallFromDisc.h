#pragma once
#include "Menu.h"
#include "GcIso/Iso.h"

namespace UI {
    class MenuInstallFromDisc: public Menu {
        public:
            MenuInstallFromDisc();
            ~MenuInstallFromDisc() {
                if(this->sprStatusBg) delete this->sprStatusBg;
            }

            virtual void draw();
            void _doInstall();

        protected:
            GX::Sprite *sprStatusBg;
            bool isDiscReady;
            UI::MenuItem *itemInstall;
            GcIso::Iso *iso;
    };
};
