#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";


void* loadGame(void *param) {
    /** Load the game main DOL.
     */
    exiPuts("Main thread running\n");
    initThreads();

    char path[1024];
    snprintf(path, sizeof(path), "sd:/%s/sys/main.dol", gameRootDir);
    FILE *dol = fopen(path, "rb");
    if(!dol) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "[stage2] Error opening main.dol: %d\n", errno);
        exiPuts(msg);
        return 1;
    }
    exiPuts("loading DOL...\n");

    DolHeader header;
    fread(&header, sizeof(DolHeader), 1, dol);
    printDolHeader(&header);
    loadDol(dol, &header);
    fclose(dol);

    exiPuts("apply patches\n");
    doPatches(&header);

    exiPuts("init DVD\n");
    initDvdHack();

    //some of the game's startup code overlaps with some
    //of the Wii OS global variables (up to 0x800031A0).
    //so we skip reading that code, and reproduce its
    //effects here.
    exiPuts("call init\r\n");
    bootGame();

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

    lwp_t mainThread;
    //u32 mainStackSize = 8192; //no idea what it should be
    u32 mainStackSize = 65536;
    LWP_CreateThread(&mainThread, loadGame, NULL,
        NULL, //0x803f8478 - mainStackSize,
        mainStackSize, 8);
    void *result;
    LWP_JoinThread(mainThread, &result);
    exiPrintf("main thread returned %08X\n", result);

    return 0;
}
