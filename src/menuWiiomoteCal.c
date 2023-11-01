/** Wii Remote Calibration submenu.
 */
#include "main.h"

#define CAL_MENU_WIDTH  400
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
    NUM_CAL_STEPS
} CalibStep;
static u8 calibStep = 0;
static u8 calibHistIdx = 0;
static bool enoughData = false;
static s16 ncOrientHist[CAL_HIST_LENGTH][3];

static const char *calInstrs[] = {
    "Hold the Nunchuk normally.",
    "Tilt the Nunchuk fully to the left.",
    "Tilt the Nunchuk fully to the right.",
    "Tilt the Nunchuk fully forward.",
    "Tilt the Nunchuk fully back.",
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
    GameWiimoteState *wp = wii ? &wii->wiimote[0] : NULL;
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
                enoughData = true;
                calibHistIdx = 0;
            }
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
        case CAL_STEP_NC_STRAIGHT:
            nunchukNeutralPos = getNunchukOrientAvg(
                &nunchukDeadzoneMin, &nunchukDeadzoneMax);
            break;

        case CAL_STEP_NC_LEFT:
            nunchukLeftPos = getNunchukOrientAvg(NULL, NULL);
            break;

        case CAL_STEP_NC_RIGHT:
            nunchukRightPos = getNunchukOrientAvg(NULL, NULL);
            break;

        case CAL_STEP_NC_FWD:
            nunchukForwardPos = getNunchukOrientAvg(NULL, NULL);
            break;

        case CAL_STEP_NC_BACK:
            nunchukBackPos = getNunchukOrientAvg(NULL, NULL);
            break;
    }
}

static void resetCalData() {
    calibHistIdx = 0;
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
            nunchukDeadzoneMin.x, nunchukDeadzoneMin.y, nunchukDeadzoneMin.z);
        OSReport("DeadMax = %+3d %+3d %+3d\r\n",
            nunchukDeadzoneMax.x, nunchukDeadzoneMax.y, nunchukDeadzoneMax.z);
        OSReport("Neutral = %+3d %+3d %+3d\r\n",
            nunchukNeutralPos.x, nunchukNeutralPos.y, nunchukNeutralPos.z);
        OSReport("Left    = %+3d %+3d %+3d\r\n",
            nunchukLeftPos.x, nunchukLeftPos.y, nunchukLeftPos.z);
        OSReport("Right   = %+3d %+3d %+3d\r\n",
            nunchukRightPos.x, nunchukRightPos.y, nunchukRightPos.z);
        OSReport("Forward = %+3d %+3d %+3d\r\n",
            nunchukForwardPos.x, nunchukForwardPos.y, nunchukForwardPos.z);
        OSReport("Back    = %+3d %+3d %+3d\r\n",
            nunchukBackPos.x, nunchukBackPos.y, nunchukBackPos.z);
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
    //Close function for Light List menu
    curMenu = &menuDebugMapEnv;
    audioPlaySound(NULL, MENU_CLOSE_SOUND);
}

Menu menuWiiCalibrate = {
    "Calibrate", 0, menu_run, menu_draw, menu_close, NULL,
};
