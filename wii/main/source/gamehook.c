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

    CONF_Init();
    char name[12];
    CONF_GetNickName(name);
    exiPrintf("Console name: \"%s\"\n", name);
}

void initGameHooks() {
    //memset(0x90000000, 0, 16*1024*1024);
    //_dspReg[5] = 0x801; //DSP reset

    SET_SCREEN_SOLID_YUV(255, 0, 148); //yellow
    udelay(500000);
    initAlloc();
    exiPuts("loader2 alloc init OK\r\n");
    initLibc();
    exiPrintf("loader2 libc init OK, IPC buf %p - %p\r\n",
        __ipcbufferLo, __ipcbufferHi);
    __lwp_wkspace_init(1*1024*1024);
    mtspr(920,(mfspr(920)&~0x40000000)); //DisableWriteGatherPipe();
    __IPC_ClntInit();
    __IOS_InitializeSubsystems();
    exiPrintf("IOS init OK.\n IPC Buf: 0x%8x - 0x%8x\n Arena1:  0x%8x - 0x%8x\n Arena2:  0x%8x - 0x%8x\n",
        __SYS_GetIPCBufferLo(), __SYS_GetIPCBufferHi(),
        *(u32*)0x8000310C, *(u32*)0x80003110,
        *(u32*)0x80003124, *(u32*)0x80003128);

    initCheckThread();
    //registerThreadForDebug(OSGetCurrentThread(),  "main");
    registerThreadForDebug((OSThread*)0x803AD848, "game");
    registerThreadForDebug((OSThread*)0x803AB118, "bsod");
    registerThreadForDebug((OSThread*)0x803A54A0, "THPaudio");
    registerThreadForDebug((OSThread*)0x803A6F08, "THPdisc");
    registerThreadForDebug((OSThread*)0x803A8348, "THPvideo");

    //CONF_Init();
    //initWiimote();
    exiPuts("loader2 IPC init OK\r\n");

    /*exiPrintf("IOS reload... (cur ver %d)\n", IOS_GetVersion());
    int r = IOS_ReloadIOS(36);
    exiPrintf("=> %d\n", r);
    //IOS_ReloadIOS(IOS_GetVersion());
    udelay(1000000);
    exiPrintf("Now on IOS %d\n", IOS_GetVersion());*/

    printConsoleInfo();
    //exiPuts("Open hid...\n");
    //static const char __hid_path[] ATTRIBUTE_ALIGN(32) = "/dev/usb/hid";
    //int r = IOS_Open(__hid_path, 0);
    //exiPrintf("HID: %d\n", r);
    //IOS_Close(r);

    SET_SCREEN_SOLID_YUV(150, 42, 202); //orange
    udelay(500000);
    initDvdHack();
    exiPuts("initDvdHack: OK; wait for DVD...\r\n");
    while(!dvdThreadReady) OSYieldThread();
    DVD_DPRINT("DVD READY\r\n");
    SET_SCREEN_SOLID_YUV(104, 212, 144); //purple
    udelay(500000);
    gameExceptionInit();
    initSaveHacks();

    memcpy((void*)0x80001800, loaderRebootCode, 6144);
    //doDspPatch();

    GameWiiInterface *wii = WII_IFACE_PTR;
    wii->updateWiimotes = updateWiimotes;
    wii->magic = WII_IFACE_MAGIC;

    //Get some of the Wii's system settings and
    //apply them to the game's defaults.
    s32 lang      = CONF_GetLanguage();
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
}
