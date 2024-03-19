#include "main.h"

WiimoteConfig wiimoteCfg[GAME_MAX_WIIMOTES];

void wiiHooksInit() {
    _DVDInit();
    wiiControlsInit();
    for(int i=0; i<GAME_MAX_WIIMOTES; i++) {
        wiimoteCfg[i].options =
            WII_SHAKE_TO_SWING |
            WII_SHAKE_TO_ROLL |
            WII_NUNCHUK_CAMERA |
            WII_NUNCHUK_STEER;
        //default calibration which works well with my controller
        wiimoteCfg[i].calNunchuk.deadMin.x =   0;
        wiimoteCfg[i].calNunchuk.deadMin.y =  -4;
        wiimoteCfg[i].calNunchuk.deadMin.z =  -4;
        wiimoteCfg[i].calNunchuk.deadMax.x =   0;
        wiimoteCfg[i].calNunchuk.deadMax.y =   4;
        wiimoteCfg[i].calNunchuk.deadMax.z =   4;
        wiimoteCfg[i].calNunchuk.pitchMin  = -31;
        wiimoteCfg[i].calNunchuk.pitchMax  =  56;
        wiimoteCfg[i].calNunchuk.rollMin   = -34;
        wiimoteCfg[i].calNunchuk.rollMax   =  72;
    }

    GameWiiInterface *wii = WII_IFACE_PTR;
    wii->drawText = drawText;
    wii->addOsdMessage = addOsdMessage;
    wii->drawBox = drawBox;
    wii->showPopupMessage = showPopupMessage;
}

void wiiLoadConfig() {
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return;

    char path[1024];
    sprintf(path, "%s/%s/wiicfg.bin", wii->saveFilePath, wii->profileName);
    FILE *file = wii->fopen(path, "rb");
    if(!file) {
        DPRINT("No wiicfg\r\n");
        return;
    }
    DPRINT("Reading Wii config\r\n");
    wii->fread(&wiimoteCfg, sizeof(WiimoteConfig), GAME_MAX_WIIMOTES, file);
    wii->fclose(file);
}
void wiiSaveConfig() {
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return;

    char path[1024];
    sprintf(path, "%s/%s/wiicfg.bin", wii->saveFilePath, wii->profileName);
    OSReport("Save path: \"%s\"\r\n", wii->saveFilePath);
    OSReport("Profile: \"%s\"\r\n", wii->profileName);
    OSReport("wiicfg path: \"%s\"\r\n", path);
    FILE *file = wii->fopen(path, "wb");
    if(!file) {
        OSReport("Can't open wiicfg.bin, error %d\r\n", wii->getErrno());
        return;
    }
    DPRINT("Saving Wii config\r\n");
    wii->fwrite(&wiimoteCfg, sizeof(WiimoteConfig), GAME_MAX_WIIMOTES, file);
    wii->fclose(file);
}

//this is working.
//we aren't actually using the normalized data anywhere
//except to display it.

bool wiiGetNunchukNormalizedOrient(int iPad, vec3f *result) {
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = wii ? &wii->wiimote[iPad] : NULL;
    if(!(wp && wp->flags & WM_FLAG_WORKING)) return false;
    if(wp->expType != WPAD_EXP_NUNCHUK) return false;
    vec3f orient = {wp->exp.nunchuk.orient[0],
        wp->exp.nunchuk.orient[1], wp->exp.nunchuk.orient[2]};
    WiiCalibrationData *cal = &wiimoteCfg[iPad].calNunchuk;

    //if     (orient.x < 0) orient.x = MIN(0, orient.x - nunchukDeadzoneMin.x);
    //else if(orient.x > 0) orient.x = MAX(0, orient.x - nunchukDeadzoneMax.x);
    //if     (orient.y < 0) orient.y = MIN(0, orient.y - nunchukDeadzoneMin.y);
    //else if(orient.y > 0) orient.y = MAX(0, orient.y - nunchukDeadzoneMax.y);
    //if     (orient.z < 0) orient.z = MIN(0, orient.z - nunchukDeadzoneMin.z);
    //else if(orient.z > 0) orient.z = MAX(0, orient.z - nunchukDeadzoneMax.z);

    //apply deadzones
    if(orient.y > cal->deadMin.y && orient.y < cal->deadMax.y) orient.y = 0;
    if(orient.z > cal->deadMin.z && orient.z < cal->deadMax.z) orient.z = 0;

    //scale
    if(cal->rollMin && cal->rollMax) {
        if     (orient.z < 0) orient.z /= ABS((float)cal->rollMin);
        else if(orient.z > 0) orient.z /= ABS((float)cal->rollMax);
    }
    if(cal->pitchMin && cal->pitchMax) {
        if     (orient.y < 0) orient.y /= ABS((float)cal->pitchMin);
        else if(orient.y > 0) orient.y /= ABS((float)cal->pitchMax);
    }

    //clamp and return
    result->x = CLAMP(orient.x, -1.0f, 1.0f);
    result->y = CLAMP(orient.y, -1.0f, 1.0f);
    result->z = CLAMP(orient.z, -1.0f, 1.0f);
    return true;
}
