#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";

GameWiiInterface wiiIface;

int main(int argc, char **argv) {
    SET_SCREEN_SOLID_YUV(255, 128, 128); //white
    exiPrintInit();
    exiPrintf("loader2 start\n");

    DolHeader *header = (DolHeader*)0x90000000;
    loadDolFromMemory(header);

    exiPrintf("game loader start; argc=%d\n", argc);
    for(int i=0; i<argc; i++) {
        exiPrintf("argv[%d] = \"%s\"\n", i, argv[i]);
    }

    u32 version = *(u32*)0x80003140;
    exiPrintf("Running on IOS%d v%d.%d\n",
        version >> 16, (version >> 8) & 0xFF, version & 0xFF);

    exiPuts("apply patches\n");
    doPatches();

    wiiIface.magic = WII_IFACE_MAGIC;
    WII_IFACE_PTR = &wiiIface;

    exiPuts("boot game\n");
    bootGame(header);

    while(1);
    return 0;
}
