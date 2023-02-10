#include "main.h"

void initGameHooks() {
    //memset(0x90000000, 0, 16*1024*1024);
    //_dspReg[5] = 0x801; //DSP reset

    SET_SCREEN_SOLID_YUV(255, 0, 148); //yellow
    udelay(500000);
    initAlloc();
    exiPuts("loader2 alloc init OK\r\n");
    initLibc();
    exiPuts("loader2 libc init OK\r\n");

    initCheckThread();
    //registerThreadForDebug(OSGetCurrentThread(),  "main");
    registerThreadForDebug((OSThread*)0x803AD848, "game");
    registerThreadForDebug((OSThread*)0x803AB118, "bsod");
    registerThreadForDebug((OSThread*)0x803A54A0, "THPaudio");
    registerThreadForDebug((OSThread*)0x803A6F08, "THPdisc");
    registerThreadForDebug((OSThread*)0x803A8348, "THPvideo");

    __lwp_wkspace_init(1*1024*1024);
    __IPC_ClntInit();
    __IOS_InitializeSubsystems();
    exiPuts("loader2 IPC init OK\r\n");

    SET_SCREEN_SOLID_YUV(150, 42, 202); //orange
    udelay(500000);
    initDvdHack();
    exiPuts("initDvdHack: OK; wait for DVD...\r\n");
    while(!dvdThreadReady) OSYieldThread();
    DVD_DPRINT("DVD READY\r\n");
    SET_SCREEN_SOLID_YUV(104, 212, 144); //purple
    udelay(500000);
    gameExceptionInit();

    memcpy((void*)0x80001800, loaderRebootCode, 6144);

    GameWiiInterface *wii = WII_IFACE_PTR;
    wii->updateWiimotes = updateWiimotes;
    wii->magic = WII_IFACE_MAGIC;
}
