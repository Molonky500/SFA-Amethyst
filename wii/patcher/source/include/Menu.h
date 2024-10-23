#pragma once
#include "MenuItem.h"
#include <vector>

namespace UI {
    /**
     * @brief Base class for menus.
     */
    class Menu {
        public:
            Menu() {
                this->iSelected = 0;
                this->iFirstDisplayed = 0;
            }

            ~Menu() {
                for(const MenuItem *item : this->items) {
                    delete item;
                }
            }

            void draw() {
                int posX = 20, posY = 40;
                u16 screenW, screenH;
                appGxGetScreenSize(&screenW, &screenH);

                int iItem=0;
                for(MenuItem *item : this->items) {
                    int sizeX=0, sizeY=0;
                    item->posX = posX;
                    item->posY = posY;
                    item->measure(&sizeX, &sizeY);
                    item->draw(iItem == this->iSelected);
                    posY += sizeY + 32;
                    if(posY >= screenH) break;
                    iItem++;
                }
            }

            void addItem(MenuItem *item) {
                this->items.push_back(item);
            }

            void move(int amount) {
                u32 nItems = this->items.size();
                if(!nItems) return;
                this->iSelected += amount;
                if(this->iSelected < 0) this->iSelected += nItems;
                this->iSelected %= nItems;
            }

            void select() {
                this->items[this->iSelected]->activate();
            }

        protected:
            std::vector<MenuItem*> items;
            int iSelected, iFirstDisplayed;
    };
};
