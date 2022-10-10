#include "main.h"

void drawWiiInputs() {
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = &wii->wiimote[0];
    if(!(wp->flags & WM_FLAG_WORKING)) return;

    int rx=320, ry=64, h=8;
    Color4b red   = {255, 0, 0, 192};
    Color4b green = {0, 255, 0, 192};
    Color4b blue  = {0, 0, 255, 192};
    hudDrawRect(rx, ry, rx+wp->accel [0]*0.1, ry+h, &red  ); ry+=h;
    hudDrawRect(rx, ry, rx+wp->accel [1]*0.1, ry+h, &green); ry+=h;
    hudDrawRect(rx, ry, rx+wp->accel [2]*0.1, ry+h, &blue ); ry+=h;
    ry += 16;
    hudDrawRect(rx, ry, rx+wp->orient[0]*10, ry+h, &red  ); ry+=h;
    hudDrawRect(rx, ry, rx+wp->orient[1]*10, ry+h, &green); ry+=h;
    hudDrawRect(rx, ry, rx+wp->orient[2]*10, ry+h, &blue ); ry+=h;
    ry += 16;
    hudDrawRect(rx, ry, rx+wp->gforce[0]*100, ry+h, &red  ); ry+=h;
    hudDrawRect(rx, ry, rx+wp->gforce[1]*100, ry+h, &green); ry+=h;
    hudDrawRect(rx, ry, rx+wp->gforce[2]*100, ry+h, &blue ); ry+=h;
    ry += 64;

    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.accel [0]*0.1, ry+h, &red  ); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.accel [1]*0.1, ry+h, &green); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.accel [2]*0.1, ry+h, &blue ); ry+=h;
            ry += 16;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.orient[0]*10, ry+h, &red  ); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.orient[1]*10, ry+h, &green); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.orient[2]*10, ry+h, &blue ); ry+=h;
            ry += 16;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.gforce[0]*100, ry+h, &red  ); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.gforce[1]*100, ry+h, &green); ry+=h;
            hudDrawRect(rx, ry, rx+wp->exp.nunchuk.gforce[2]*100, ry+h, &blue ); ry+=h;
            ry += 16;
            break;
        }

        default: break;
    }
}
