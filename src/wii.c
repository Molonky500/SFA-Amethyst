#include "main.h"

u32 wiiOptions =
    WII_SHAKE_TO_SWING |
    WII_SHAKE_TO_ROLL |
    WII_NUNCHUK_CAMERA |
    WII_NUNCHUK_STEER;

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
