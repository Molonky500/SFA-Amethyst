/** Wii Remote submenu.
 */
#include "main.h"

void menuWiimoteShakeSwing_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiimoteCfg[0].options & WII_SHAKE_TO_SWING) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteShakeSwing_select(const MenuItem *self, int amount) {
    wiimoteCfg[0].options ^= WII_SHAKE_TO_SWING;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteShakeRoll_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiimoteCfg[0].options & WII_SHAKE_TO_ROLL) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteShakeRoll_select(const MenuItem *self, int amount) {
    wiimoteCfg[0].options ^= WII_SHAKE_TO_ROLL;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteNunCam_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiimoteCfg[0].options & WII_NUNCHUK_CAMERA) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteNunCam_select(const MenuItem *self, int amount) {
    wiimoteCfg[0].options ^= WII_NUNCHUK_CAMERA;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteNunSteer_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name),
        (wiimoteCfg[0].options & WII_NUNCHUK_STEER) ? T("On") : T("Off"));
    menuDrawText(str, x, y, selected);
}
void menuWiimoteNunSteer_select(const MenuItem *self, int amount) {
    wiimoteCfg[0].options ^= WII_NUNCHUK_STEER;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}


void menuWiimoteCamScaleX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXScale);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamScaleX_select(const MenuItem *self, int amount) {
    nunchukXScale += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteCamScaleY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYScale);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamScaleY_select(const MenuItem *self, int amount) {
    nunchukYScale += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}


void menuWiimoteCamMinX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXMin);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMinX_select(const MenuItem *self, int amount) {
    nunchukXMin += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteCamMinY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYMin);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMinY_select(const MenuItem *self, int amount) {
    nunchukYMin += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}


void menuWiimoteCamMaxX_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukXMax);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMaxX_select(const MenuItem *self, int amount) {
    nunchukXMax += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteCamMaxY_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    sprintf(str, self->fmt, T(self->name), nunchukYMax);
    menuDrawText(str, x, y, selected);
}
void menuWiimoteCamMaxY_select(const MenuItem *self, int amount) {
    nunchukYMax += (float)amount / 10.0f;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuWiimoteCalibrate_select(const MenuItem *self, int amount) {
    if(amount) return;
    curMenu = &menuWiiCalibrate;
    audioPlaySound(NULL, MENU_OPEN_SOUND);
}

void menuWiimote_close(const Menu *self) {
    wiiSaveConfig();
    curMenu = &menuControlSettings;
    audioPlaySound(NULL, MENU_CLOSE_SOUND);
}

Menu menuWiimote = {
    "Wii Remote", 0,
    genericMenu_run, genericMenu_draw, menuWiimote_close,
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
    "Calibrate", "%s", genericMenuItem_draw, menuWiimoteCalibrate_select,
    NULL,
};
