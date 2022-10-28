#include "main.h"

PopupMessage popupMsgs[MAX_POPUP_MSGS];

bool motionBlurHook() {
    //replaces a bl to shouldForceMotionBlur()
    bool force = shouldForceMotionBlur();
    if(force) return force;
    if(!bRumbleBlur) return false;

    static float prevRumbleTimer = 0;
    if(rumbleTimer > 0) {
        prevRumbleTimer = rumbleTimer;
        float rumble = (rumbleTimer + 48) * 2;
        if(rumble > 120) rumble = 120;
        motionBlurIntensity = 255 - rumble;
        return true;
    }
    else if(prevRumbleTimer > 0) {
        //255 is min, 0 is max
        motionBlurIntensity = 255;
    }
    prevRumbleTimer = rumbleTimer;
    return false;
}

void showPopupMsg(const char *text, float time) {
    for(int i=0; i<MAX_POPUP_MSGS; i++) {
        if(popupMsgs[i].time <= 0.0f) {
            popupMsgs[i].time = time;
            popupMsgs[i].text = text;
            return;
        }
    }
    OSReport("Too many popup messages! Dropped: %s\n", text);
}

static void drawPopupMessages() {
    int popupX = 40;
    int popupY = SCREEN_HEIGHT - (LINE_HEIGHT * 2);
    for(int i=0; i<MAX_POPUP_MSGS; i++) {
        if(popupMsgs[i].time > 0.0f) {
            popupMsgs[i].time -= timeDelta;
            Color4b color = {.r=255, .g=255, .b=255,
                .a = (int)(MAX(popupMsgs[i].time, 1.0f) * 255.0f)};
            drawColorText(popupMsgs[i].text, popupX, popupY, color);
            popupY -= LINE_HEIGHT;
        }
    }
}

void hudDrawHook(int p1, int p2, int p3) {
    drawPopupMessages();
    if(curGameText != -1 || !(cameraFlags & CAM_FLAG_NO_HUD)) {
        GameUI_hudDraw(p1, p2, p3);
    }
    else { //still manually render a few things
        //XXX draw the aiming crosshair for staff
        pauseMenuDraw(p1, p2, p3);
        pauseMenuDrawText();
        if(mapScreenVisible) mapScreenDrawHud();
        if(timeListVisible) timeListDraw();
        hudDrawAirMeter();
        if(cMenuEnabled && cMenuFadeCounter) hudDrawButtons(p1, p2, p3);
        fearTestMeterDraw();
    }
}
