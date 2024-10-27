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
                GX::getScreenSize(screenW, screenH);

                int iItem=0;
                for(MenuItem *item : this->items) {
                    int sizeX=0, sizeY=0;
                    item->setPos(posX, posY)
                        ->getSize(sizeX, sizeY);
                    item->draw(iItem == this->iSelected);
                    posY += sizeY;
                    if(posY >= screenH) break;
                    iItem++;
                }
            }

            void addItem(MenuItem *item) {
                this->items.push_back(item);
            }

            void move(int amount) {
                if(!amount) return;
                u32 nItems = this->items.size();
                if(!nItems) return;

                this->iSelected += amount;
                if(this->iSelected < 0) this->iSelected += nItems;
                this->iSelected %= nItems;

                //if this item is disabled, move on to another.
                //but don't get stuck if they're all disabled.
                int skip = (amount < 0) ? -1 : 1;
                u32 tries=0;
                while(++tries < nItems
                && !this->items[this->iSelected]->enabled) {
                    this->iSelected += skip;
                    if(this->iSelected < 0) this->iSelected += nItems;
                    this->iSelected %= nItems;
                }
            }

            void select() {
                this->items[this->iSelected]->activate();
            }

            void handlePointer(int x, int y);

        protected:
            std::vector<MenuItem*> items;
            int iSelected, iFirstDisplayed;
    };
};
