/** Wii Remote Calibration submenu.
 */
#include "main.h"

#define CAL_MENU_WIDTH  500
#define CAL_MENU_HEIGHT 150
#define CAL_MENU_XPOS ((SCREEN_WIDTH/2) - (CAL_MENU_WIDTH / 2))
#define CAL_MENU_YPOS ((SCREEN_HEIGHT/2) - (CAL_MENU_HEIGHT / 2))
#define CAL_HIST_LENGTH 60

typedef enum {
    CAL_STEP_NC_STRAIGHT,
    CAL_STEP_NC_LEFT,
    CAL_STEP_NC_RIGHT,
    CAL_STEP_NC_FWD,
    CAL_STEP_NC_BACK,
    CAL_STEP_COMPLETE,
    NUM_CAL_STEPS
} CalibStep;
static u8 calibStep = 0;
static u8 calibHistIdx = 0;
static u8 calibWrapCnt = 0;
static bool enoughData = false;
static s16 ncOrientHist[CAL_HIST_LENGTH][3];
static u8 iPad = 0;

static const char *calInstrs[] = {
    "Point the Nunchuk forward, with the\r\nanalog stick pointed up.",
    "Point the Nunchuk forward, with the\r\nanalog stick pointed left.",
    "Point the Nunchuk forward, with the\r\nanalog stick pointed right.",
    "Point the Nunchuk toward the ground, with the\r\nanalog stick pointed forward.",
    "Point the Nunchuk toward the sky, with the\r\nanalog stick pointed at you.",
    "Calibration successful."
};

static vec3s getNunchukOrientAvg(vec3s *outMin, vec3s *outMax) {
    vec3s result = {0,0,0};
    vec3s vMin   = {0,0,0};
    vec3s vMax   = {0,0,0};
    for(int i=0; i<CAL_HIST_LENGTH; i++) {
        result.x += ncOrientHist[i][0];
        result.y += ncOrientHist[i][1];
        result.z += ncOrientHist[i][2];
        vMin.x = MIN(vMin.x, ncOrientHist[i][0]);
        vMin.y = MIN(vMin.y, ncOrientHist[i][1]);
        vMin.z = MIN(vMin.z, ncOrientHist[i][2]);
        vMax.x = MAX(vMax.x, ncOrientHist[i][0]);
        vMax.y = MAX(vMax.y, ncOrientHist[i][1]);
        vMax.z = MAX(vMax.z, ncOrientHist[i][2]);
    }
    result.x /= CAL_HIST_LENGTH;
    result.y /= CAL_HIST_LENGTH;
    result.z /= CAL_HIST_LENGTH;
    if(outMin) *outMin = vMin;
    if(outMax) *outMax = vMax;
    return result;
}

static void menu_draw(Menu *self) {
    menuAnimFrame++;
    drawMenuBox(CAL_MENU_XPOS, CAL_MENU_YPOS, CAL_MENU_WIDTH, CAL_MENU_HEIGHT);
    menuDrawText(calInstrs[calibStep], CAL_MENU_XPOS + 8, CAL_MENU_YPOS + 8, false);
    if(enoughData) {
        menuDrawText("A:Next  B:Back", CAL_MENU_XPOS + 8,
            CAL_MENU_YPOS + CAL_MENU_HEIGHT - 24,
            false);
    }
}

static void updateCalibStep() {
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = wii ? &wii->wiimote[iPad] : NULL;
    if(!(wp && wp->flags & WM_FLAG_WORKING)) return;
    switch(calibStep) {
        case CAL_STEP_NC_STRAIGHT:
        case CAL_STEP_NC_LEFT:
        case CAL_STEP_NC_RIGHT:
        case CAL_STEP_NC_FWD:
        case CAL_STEP_NC_BACK:
            ncOrientHist[calibHistIdx][0] = wp->exp.nunchuk.orient[0];
            ncOrientHist[calibHistIdx][1] = wp->exp.nunchuk.orient[1];
            ncOrientHist[calibHistIdx][2] = wp->exp.nunchuk.orient[2];
            calibHistIdx++;
            if(calibHistIdx >= CAL_HIST_LENGTH) {
                calibWrapCnt++;
                if(calibWrapCnt > 1) enoughData = true;
                calibHistIdx = 0;
            }
            break;

        case CAL_STEP_COMPLETE:
            enoughData = true;
            break;
    }

    vec3s pos, vMin, vMax;
    pos = getNunchukOrientAvg(&vMin, &vMax);
    debugPrintf(DPRINT_FIXED "Pos: %4d %4d %4d Min: %4d %4d %4d",
        pos.x, pos.y, pos.z,
        vMin.x, vMin.y, vMin.z);
    debugPrintf(" Max: %4d %4d %4d" DPRINT_NOFIXED "\n",
        vMax.x, vMax.y, vMax.z);
}

static void applyCalibStep() {
    switch(calibStep) {
        case CAL_STEP_NC_STRAIGHT: {
            getNunchukOrientAvg(
                &wiimoteCfg[iPad].calNunchuk.deadMin,
                &wiimoteCfg[iPad].calNunchuk.deadMax);
            break;
        }

        case CAL_STEP_NC_LEFT:
            wiimoteCfg[iPad].calNunchuk.rollMin = getNunchukOrientAvg(NULL, NULL).z;
            break;

        case CAL_STEP_NC_RIGHT:
            wiimoteCfg[iPad].calNunchuk.rollMax = getNunchukOrientAvg(NULL, NULL).z;
            break;

        case CAL_STEP_NC_FWD:
            wiimoteCfg[iPad].calNunchuk.pitchMax = getNunchukOrientAvg(NULL, NULL).y;
            break;

        case CAL_STEP_NC_BACK:
            wiimoteCfg[iPad].calNunchuk.pitchMin = getNunchukOrientAvg(NULL, NULL).y;
            break;
    }
}

static void resetCalData() {
    calibHistIdx = 0;
    calibWrapCnt = 0;
    enoughData = false;
}

static void prevStep(Menu *self) {
    resetCalData();
    if(calibStep) calibStep--;
    else self->close(curMenu);
}

static void nextStep(Menu *self) {
    resetCalData();
    calibStep++;
    if(calibStep >= NUM_CAL_STEPS) {
        calibStep = 0;
        self->close(curMenu);
        OSReport("DeadMin = %+3d %+3d %+3d\r\n",
            wiimoteCfg[iPad].calNunchuk.deadMin.x,
            wiimoteCfg[iPad].calNunchuk.deadMin.y,
            wiimoteCfg[iPad].calNunchuk.deadMin.z);
        OSReport("DeadMax = %+3d %+3d %+3d\r\n",
            wiimoteCfg[iPad].calNunchuk.deadMax.x,
            wiimoteCfg[iPad].calNunchuk.deadMax.y,
            wiimoteCfg[iPad].calNunchuk.deadMax.z);
        OSReport("Pitch   = %+3d %+3d\r\n",
            wiimoteCfg[iPad].calNunchuk.pitchMin,
            wiimoteCfg[iPad].calNunchuk.pitchMax);
        OSReport("Roll    = %+3d %+3d\r\n",
            wiimoteCfg[iPad].calNunchuk.rollMin,
            wiimoteCfg[iPad].calNunchuk.rollMax);
    }
}

static void menu_run(Menu *self) {
    if(buttonsJustPressed == PAD_BUTTON_B) {
        prevStep(self);
    }
    else if(enoughData && buttonsJustPressed == PAD_BUTTON_A) {
        applyCalibStep();
        nextStep(self);
    }
    else updateCalibStep();
}

static void menu_close(const Menu *self) {
    curMenu = &menuWiimote;
    audioPlaySound(NULL, MENU_CLOSE_SOUND);
}

Menu menuWiiCalibrate = {
    "Calibrate", 0, menu_run, menu_draw, menu_close, NULL,
};
