#include "main.h"

//XXX figure out how to do this for 4 controllers?
u32 wiiOptions =
    WII_SHAKE_TO_SWING |
    WII_SHAKE_TO_ROLL |
    WII_NUNCHUK_CAMERA |
    WII_NUNCHUK_STEER;
vec3s nunchukNeutralPos; //orient when held normally
vec3s nunchukDeadzoneMin; //minimum values for "held normally"
vec3s nunchukDeadzoneMax; //maximum values for "held normally"
vec3s nunchukLeftPos; //orient when tilted left
vec3s nunchukRightPos;
vec3s nunchukForwardPos;
vec3s nunchukBackPos;

void wiiHooksInit() {
    _DVDInit();
    wiiControlsInit();
}

void wiiLoadConfig() {
    //TODO: read wiiOptions from a file
}
void wiiSaveConfig() {
    //TODO: write wiiOptions to a file
}

//this is working.
//a bit wasteful because we store a bunch of unused values.
//we also aren't actually using the normalized data anywhere
//except to display it.
//should condense into a calibration struct that we can reuse
//for the Wiimote's own gyro.

bool wiiGetNunchukNormalizedOrient(int iPad, vec3f *result) {
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = wii ? &wii->wiimote[iPad] : NULL;
    if(!(wp && wp->flags & WM_FLAG_WORKING)) return false;
    if(wp->expType != WPAD_EXP_NUNCHUK) return false;
    vec3f orient = {wp->exp.nunchuk.orient[0],
        wp->exp.nunchuk.orient[1], wp->exp.nunchuk.orient[2]};

    //if     (orient.x < 0) orient.x = MIN(0, orient.x - nunchukDeadzoneMin.x);
    //else if(orient.x > 0) orient.x = MAX(0, orient.x - nunchukDeadzoneMax.x);
    //if     (orient.y < 0) orient.y = MIN(0, orient.y - nunchukDeadzoneMin.y);
    //else if(orient.y > 0) orient.y = MAX(0, orient.y - nunchukDeadzoneMax.y);
    //if     (orient.z < 0) orient.z = MIN(0, orient.z - nunchukDeadzoneMin.z);
    //else if(orient.z > 0) orient.z = MAX(0, orient.z - nunchukDeadzoneMax.z);

    //apply deadzones
    if(orient.x > nunchukDeadzoneMin.x && orient.x < nunchukDeadzoneMax.x) orient.x = 0;
    if(orient.y > nunchukDeadzoneMin.y && orient.y < nunchukDeadzoneMax.y) orient.y = 0;
    if(orient.z > nunchukDeadzoneMin.z && orient.z < nunchukDeadzoneMax.z) orient.z = 0;

    if(!nunchukLeftPos.z) nunchukLeftPos.z = 1;
    if(!nunchukRightPos.z) nunchukRightPos.z = 1;
    if(!nunchukBackPos.y) nunchukBackPos.y = 1;
    if(!nunchukForwardPos.y) nunchukForwardPos.y = 1;

    //scale
    if     (orient.z < 0) orient.z /= ABS((float)nunchukLeftPos.z);
    else if(orient.z > 0) orient.z /= ABS((float)nunchukRightPos.z);
    if     (orient.y < 0) orient.y /= ABS((float)nunchukBackPos.y);
    else if(orient.y > 0) orient.y /= ABS((float)nunchukForwardPos.y);

    //X is ??? always zero? (yaw)
    //Y is forward/back (pitch)
    //Z is left/right (roll)

    //clamp and return
    result->x = CLAMP(orient.x, -1.0f, 1.0f);
    result->y = CLAMP(orient.y, -1.0f, 1.0f);
    result->z = CLAMP(orient.z, -1.0f, 1.0f);
    return true;
}
