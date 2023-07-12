#include "main.h"

uint OSGetSoundMode_hook() {
    s32 soundMode = CONF_GetSoundMode();
    switch(soundMode) {
        case CONF_SOUND_MONO: return 0;
        case CONF_SOUND_STEREO: return 1;
        //must default to stereo, or else surround becomes mono.
        default: return 1;
    }
}

void saveGame_initialize_hook() {
    //actually takes a param but doesn't use it
    void (*origFunc)(void) = 0x800ea078;
    origFunc();
    exiPrintf("CONF: Aspect:%d Sound:%d Rumble:%d\n",
        CONF_GetAspectRatio(),
        CONF_GetSoundMode(),
        CONF_GetPadMotorMode());
    (*(u8*)0x803a31ca) = (CONF_GetAspectRatio() == CONF_ASPECT_16_9);

    s32 soundMode = CONF_GetSoundMode();
    u8  gameSoundMode = 0; //stereo
    switch(soundMode) {
        case CONF_SOUND_MONO: gameSoundMode = 2; break;
        case CONF_SOUND_STEREO: gameSoundMode = 0; break;
        case CONF_SOUND_SURROUND: gameSoundMode = 1; break;
        //no default setting for headphones
    }
    (*(u8*)0x803a31cd) = gameSoundMode;
    (*(u8*)0x803a31cc) = CONF_GetPadMotorMode() ? 1 : 0;
}
