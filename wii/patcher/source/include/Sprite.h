#pragma once
#include "Texture.h"

namespace GX {
    class Sprite {
        public:
            Sprite(std::shared_ptr<Texture> tex) {
                this->setTexture(tex);
                this->posX = 0;
                this->posY = 0;
                this->scale = 1.0f;
                this->color = {0xFF, 0xFF, 0xFF, 0xFF};
            }

            std::shared_ptr<Texture> getTexture() {
                return this->texture;
            }
            Sprite* setTexture(std::shared_ptr<Texture> tex) {
                this->texture = tex;
                if(tex) {
                    tex->getSize(&this->sizeX, &this->sizeY);
                }
                return this;
            }

            Sprite* getSize(int &x, int &y) {
                x = this->sizeX;
                y = this->sizeY;
                return this;
            }

            Sprite* getPos(int &x, int &y) {
                x = this->posX;
                y = this->posY;
                return this;
            }
            Sprite* setPos(int x, int y) {
                this->posX = x;
                this->posY = y;
                return this;
            }

            float getOpacity() {
                return this->color.a / 255.0f;
            }
            Sprite* setOpacity(float op) {
                this->color.a = 255 * op;
                return this;
            }

            void draw() {
                if(!this->texture) return;
                this->_drawTexture(&*this->texture, 0, 0);
            }

            bool hitTest(int x, int y) {
                return x >= this->posX && x < this->posX + this->sizeX
                    && y >= this->posY && y < this->posY + this->sizeY;
            }

        protected:
            std::shared_ptr<GX::Texture> texture;
            int posX, posY;
            u16 sizeX, sizeY;
            float scale;
            GX::Color color;

            void _drawTexture(GX::Texture *tex, int offsX, int offsY) {
                tex->select();
                GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

                //top left
                GX_Position2s16(this->posX+offsX, this->posY+offsY);
                GX_Color1u32(this->color.value);
                GX_TexCoord2s16(0, 0);

                //top right
                GX_Position2s16(this->posX+this->sizeX+offsX,
                    this->posY+offsY);
                GX_Color1u32(this->color.value);
                GX_TexCoord2s16(this->sizeX, 0);

                //bottom right
                GX_Position2s16(this->posX+this->sizeX+offsX,
                    this->posY+this->sizeY+offsY);
                GX_Color1u32(this->color.value);
                GX_TexCoord2s16(this->sizeX, this->sizeY);

                //bottom left
                GX_Position2s16(this->posX+offsX,
                    this->posY+this->sizeY+offsY);
                GX_Color1u32(this->color.value);
                GX_TexCoord2s16(0, this->sizeY);

                GX_End();
            }
    };
};
