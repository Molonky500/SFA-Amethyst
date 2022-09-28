#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";


void* loadGame(void *param) {
    /** Load the game main DOL.
     */
    exiPuts("Main thread running\n");
    //initThreads();

    DolHeader *header = (DolHeader*)0x90000000;
    loadDolFromMemory(header);

    exiPuts("apply patches\n");
    doPatches();

    //exiPuts("init DVD\n");
    //initDvdHack();

    //some of the game's startup code overlaps with some
    //of the Wii OS global variables (up to 0x800031A0).
    //so we skip reading that code, and reproduce its
    //effects here.
    exiPuts("call init\r\n");
    bootGame(header);

    return NULL;
}

int main(int argc, char **argv) {
    //move mem2lo so allocations don't get clobbered
    //(*(u32*)0x80003124) = 0x92800000;
    //SYS_SetArena1Lo((void*)0x92800000);
    //SYS_SetArena1Hi((void*)0x92900000);
    //SYS_SetArena2Lo((void*)0x92900000);
    //SYS_SetArena2Hi((void*)0x92FFFFFF);

    init();
    exiPrintf("game loader start; arenas = %08X - %08X, %08X - %08X\n",
        SYS_GetArena1Lo(), SYS_GetArena1Hi(),
        SYS_GetArena2Lo(), SYS_GetArena2Hi());
    exiPrintf("argc=%d\n", argc);
    for(int i=0; i<argc; i++) {
        exiPrintf("argv[%d] = \"%s\"\n", i, argv[i]);
    }

    /*lwp_t mainThread;
    u32 mainStackSize = 8192; //no idea what it should be
    //u32 mainStackSize = 65536;
    memset(0x803f8478 - mainStackSize, 0xAAAAAAAA, mainStackSize);
    LWP_CreateThread(&mainThread, loadGame, NULL,
        0x803f8478 - mainStackSize,
        mainStackSize, 8);
    void *result;
    LWP_JoinThread(mainThread, &result);
    exiPrintf("main thread returned %08X\n", result);*/
    loadGame(NULL);
    while(1);

    return 0;
}
