#include "main.h"

extern u8 __ipcbufferLo[], __ipcbufferHi[];

void printConsoleInfo() {
    exiPrintf("MEM1 Size:    0x%8X (Sim: 0x%8X)\n",
        *(u32*)0x80000028, *(u32*)0x800000F0);
    exiPrintf("MEM1 Size2:   0x%8X (Sim: 0x%8X)\n",
        *(u32*)0x80003100, *(u32*)0x80003104);
    exiPrintf("MEM2 Size:    0x%8X (Sim: 0x%8X)\n",
        *(u32*)0x80003118, *(u32*)0x8000311C);
    exiPrintf("ARAM Size:    0x%8X\n", *(u32*)0x800000D0);
    exiPrintf("Arena1:       0x%8X - 0x%8X\n",
        *(u32*)0x80000030, *(u32*)0x80000034);
    exiPrintf("Arena1_2:     0x%8X - 0x%8X\n",
        *(u32*)0x8000310C, *(u32*)0x80003110);
    exiPrintf("Arena2:       0x%8X - 0x%8X\n",
        *(u32*)0x80003124, *(u32*)0x80003128);
    exiPrintf("PPC Mem2 End: 0x%8X\n",
        *(u32*)0x80003120);
    exiPrintf("IPC Buffer:   0x%8X - 0x%8X\n",
        *(u32*)0x80003130, *(u32*)0x80003134);
    exiPrintf("Bus Clock:    %3dMHz  CPU Clock: %3dMHz\n",
        (*(u32*)0x800000F8) / 1000000,
        (*(u32*)0x800000FC) / 1000000);

    //CONF_Init();
    char name[12];
    CONF_GetNickName(name);
    exiPrintf("Console name: \"%s\"\n", name);
}

int getErrno() {
    return errno;
}

void initGameHooks() {
    //_dspReg[5] = 0x801; //DSP reset

    exiPuts("Game init hook OK!\n");
    __UnmaskIrq(IM_PI_ACR); //enable IOS IPC
    _initIos();
    //L2Enhance();

    //if we init Wiimote after FAT, it likes to fail.
    exiPuts("About to init Wiimote\n");
    if(initWiimote()) {
        exiPuts("Wiimote init failed\n");
    }
    else exiPuts("Wiimote init OK\n");

    //init filesystem
    exiPuts("Init FAT...\n");
    if(!fatInitDefault()) {
        exiPuts("FAT init failed\n");
        return 1;
    }
    exiPuts("Init game files...\n");
    initGameFiles(save_argc > 0 ? save_argv[0] : NULL);

    //back up the loader reboot code since loading
    //the game DOL will overwrite it.
    loaderRebootCode = malloc(6144);
    if(loaderRebootCode) {
        memcpy(loaderRebootCode, (void*)0x80001800, 6144);
    }

    exiPuts("Init check-thread...\n");
    initCheckThread();
    //registerThreadForDebug(OSGetCurrentThread(),  "main");
    registerThreadForDebug((OSThread*)0x803AD848, "game");
    registerThreadForDebug((OSThread*)0x803AB118, "bsod");
    registerThreadForDebug((OSThread*)0x803A54A0, "THPaudio");
    registerThreadForDebug((OSThread*)0x803A6F08, "THPdisc");
    registerThreadForDebug((OSThread*)0x803A8348, "THPvideo");
    initStreamThread();

    exiPuts("loader2 IPC init OK\r\n");
    printConsoleInfo();

    SET_SCREEN_SOLID_YUV(104, 212, 144); //purple
    //udelay(500000);
    exiPuts("Init exception...\n");
    gameExceptionInit();
    exiPuts("Init save...\n");
    initSaveHacks();

    SET_SCREEN_SOLID_YUV(150, 42, 202); //orange
    //udelay(500000);
    exiPuts("Init DVD...\n");
    initDvdHack();
    exiPuts("initDvdHack: OK; wait for DVD...\r\n");
    while(!dvdThreadReady) OSYieldThread();
    DVD_DPRINT("DVD READY\r\n");
    _ipcReg[9] = 0; //stop forcing screen color

    if(loaderRebootCode) {
        exiPrintf("Restoring loader reboot code from 0x%x\n", loaderRebootCode);
        memcpy((void*)0x80001800, loaderRebootCode, 6144);
        free(loaderRebootCode);
        loaderRebootCode = NULL;
    }
    exiPuts("Init DSP...\n");
    doDspPatch();

    exiPuts("Init Wii iface...\r\n");
    GameWiiInterface *wii = WII_IFACE_PTR;
    wii->updateWiimotes = updateWiimotes;
    wii->fopen          = fopen;
    wii->fclose         = fclose;
    wii->fread          = fread;
    wii->fwrite         = fwrite;
    wii->opendir        = opendir;
    wii->closedir       = closedir;
    wii->readdir        = readdir;
    wii->rewinddir      = rewinddir;
    wii->seekdir        = seekdir;
    wii->telldir        = telldir;
    wii->stat           = stat;
    wii->getErrno       = getErrno;
    wii->gameRootDir    = gameRootDir;
    wii->magic          = WII_IFACE_MAGIC;
    exiPrintf("sizeof(FILE) = %d, DIR=%d.\r\n", sizeof(FILE), sizeof(DIR));

    //Get some of the Wii's system settings and
    //apply them to the game's defaults.
    s32 lang = CONF_GetLanguage();
    switch(lang) { //translate to game's language enum
        case CONF_LANG_JAPANESE: lang = 4; break;
        case CONF_LANG_ENGLISH:  lang = 0; break;
        case CONF_LANG_GERMAN:   lang = 2; break;
        case CONF_LANG_FRENCH:   lang = 1; break;
        case CONF_LANG_SPANISH:  lang = 5; break;
        case CONF_LANG_ITALIAN:  lang = 3; break;
        default: lang = 0; break; //default to English
    }
    wii->language = lang; //game will set it when ready

    //patch default option in progressive scan prompt
    (*(u32*)0x8001fa6c) = 0x3ba00000 | (CONF_GetProgressiveScan() ? 1 : 0);

    exiPuts("Patching allocator\r\n");
    //attempt to fix heap metrics
    //but only breaks allocation instead
    /*u32 patches[] = {
        0x800239ec, 0x38A5FFFC, //subi      r5,r5,0x4
        0x80023a0c, 0x8003FFF8, //lwz       r0,-0x8(size2)
        0x80023f18, 0x9085FFF8, //stw       size,-0x8(slots)
        0x80023f1c, 0x9005FFFC, //stw       r0,-0x4(slots)
        0};
    for(int i=0; patches[i]; i += 2) {
        *(u32*)patches[i] = patches[i+1];
        DCFlushRange((void*)patches[i], 4);
        ICInvalidateRange((void*)patches[i], 4);
    }*/
    hookBranch(0x80020e58, initHeaps_hook, 1, 0);
    *(u32*)0x80253628 = 0x578300F4; //EXIDma: use full Wii address space

    exiPuts("Init STM...\r\n");
    STM_RegisterEventHandler(MyStmHandler);
    exiPuts("Init OK\n");
}
