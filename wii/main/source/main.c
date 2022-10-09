#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";


int main(int argc, char **argv) {
    //_ipcReg[9] = (y << 8) | (v << 16) | (u << 24) | 1;
    _ipcReg[9] = (76 << 8) | (255 << 16) | (84 << 24) | 1;
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

    exiPuts("boot game\n");
    _ipcReg[9] = (149 << 8) | (21 << 16) | (43 << 24) | 1;
    bootGame(header);

    while(1);
    return 0;
}
