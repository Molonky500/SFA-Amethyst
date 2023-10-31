/** Wii Remote submenu.
 */
#include "main.h"

void menuWiimoteShakeSwing_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiiOptions & WII_SHAKE_TO_SWING) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteShakeSwing_select(const MenuItem *self, int amount) {
    wiiOptions ^= WII_SHAKE_TO_SWING;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteShakeRoll_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiiOptions & WII_SHAKE_TO_ROLL) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteShakeRoll_select(const MenuItem *self, int amount) {
    wiiOptions ^= WII_SHAKE_TO_ROLL;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteNunCam_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiiOptions & WII_NUNCHUK_CAMERA) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteNunCam_select(const MenuItem *self, int amount) {
    wiiOptions ^= WII_NUNCHUK_CAMERA;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteNunSteer_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiiOptions & WII_NUNCHUK_STEER) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteNunSteer_select(const MenuItem *self, int amount) {
    wiiOptions ^= WII_NUNCHUK_STEER;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}


void menuWiimoteCamScaleX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXScale);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamScaleX_select(const MenuItem *self, int amount) {
    nunchukXScale += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteCamScaleY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYScale);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamScaleY_select(const MenuItem *self, int amount) {
    nunchukYScale += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}


void menuWiimoteCamMinX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXMin);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMinX_select(const MenuItem *self, int amount) {
    nunchukXMin += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteCamMinY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYMin);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMinY_select(const MenuItem *self, int amount) {
    nunchukYMin += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}


void menuWiimoteCamMaxX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXMax);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMaxX_select(const MenuItem *self, int amount) {
    nunchukXMax += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

void menuWiimoteCamMaxY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYMax);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMaxY_select(const MenuItem *self, int amount) {
    nunchukYMax += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
    wiiSaveConfig();
}

Menu menuWiimote = {
    "Wii Remote", 0,
    genericMenu_run, genericMenu_draw, controlSubMenu_close,
    "Shake Remote to Swing","%s: %s", menuWiimoteShakeSwing_draw, menuWiimoteShakeSwing_select,
    "Shake Nunchuk to Roll","%s: %s", menuWiimoteShakeRoll_draw, menuWiimoteShakeRoll_select,
    "Nunchuk Camera Control","%s: %s", menuWiimoteNunCam_draw, menuWiimoteNunCam_select,
    "Nunchuk Steering Control","%s: %s", menuWiimoteNunSteer_draw, menuWiimoteNunSteer_select,
    "Cam Tilt X Scale","%s: %1.1fx", menuWiimoteCamScaleX_draw, menuWiimoteCamScaleX_select,
    "Cam Tilt Y Scale","%s: %1.1fx", menuWiimoteCamScaleY_draw, menuWiimoteCamScaleY_select,
    "Cam Tilt X Min","%s: %1.1fx", menuWiimoteCamMinX_draw, menuWiimoteCamMinX_select,
    "Cam Tilt Y Min","%s: %1.1fx", menuWiimoteCamMinY_draw, menuWiimoteCamMinY_select,
    "Cam Tilt X Max","%s: %1.1fx", menuWiimoteCamMaxX_draw, menuWiimoteCamMaxX_select,
    "Cam Tilt Y Max","%s: %1.1fx", menuWiimoteCamMaxY_draw, menuWiimoteCamMaxY_select,
    NULL,
};
