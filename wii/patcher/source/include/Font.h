#pragma once
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include "Gx.h"

namespace GX {
    class Font {
        public:
            Font();

            Font* getPos(int x, int y) {
                x = this->posX;
                y = this->posY;
                return this;
            }
            Font* setPos(int x, int y) {
                this->posX = x;
                this->posY = y;
                return this;
            }

            int getSize() {
                return this->size;
            }
            Font* setSize(int size) {
                this->size = size;
                return this;
            }

            Color getColor() {
                return this->color;
            }
            Font* setColor(Color color) {
                this->color = color;
                return this;
            }

            int drawString(const char *text);
            void measureString(const char *text, int &outW, int &outH);

        protected:
            int posX, posY, size;
            Color color;
            sys_fontheader *fontdata;
            GXTexObj fonttex;

            void selectTexture();
            void drawCell(s16 x1, s16 y1, s16 s1, s16 t1);
            int processString(const char *text, bool shouldDraw,
                int &outY);
    };
};

}; //extern "C"
