#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";

GameWiiInterface wiiIface;
u8 loaderRebootCode[6144];
bool isLaunchedFromLoader = false;
bool gIsSystemShuttingDown = false;
bool isReset = true; //is this shutdown really a reboot?

int main(int argc, char **argv) {
    SET_SCREEN_SOLID_YUV(255, 128, 128); //white
    if(argc > 0) isLaunchedFromLoader = true;
    //else we're launched in Dolphin from command line or something
    //and there's no loader to exit to.

    exiPrintInit();
    exiPrintf("loader2 start\n");

    memcpy(loaderRebootCode, (void*)0x80001800, 6144);
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

    STM_RegisterEventHandler(MyStmHandler);

    exiPuts("boot game\n");
    bootGame(header); //doesn't return
    while(1);
    return 0;
}

void MyStmHandler(u32 event) {
    switch(event) {
        case STM_EVENT_POWER:
            isReset = false;
            //fall thru

        case STM_EVENT_RESET:
            // *(float*)0x803dcb00 = 4.0f; //reset fadeout timer
            //we get FPU Unavailable exception...
            // *(u32*)0x803dcb00 = 0x40800000;
            *(u8*)0x803dcca6 = 1; //shouldReset
            *(u8*)0x803dca3e = 1; //shouldResetNextFrame
            exiPuts("Told game to reboot!\n");
            break;

        default:
            printf("Unknown STM event %08X\n", event);
    }
}

void OSRebootHook() {
    gIsSystemShuttingDown = true;
    exiPuts(isReset ? "*** SYSTEM REBOOTING\n" :
        "*** SYSTEM SHUTTING DOWN\n");
    OSCancelThread(&hackDvdThread);
    SET_SCREEN_SOLID_YUV(106, 139, 94); //teal
    WPAD_Shutdown();
    fatUnmount("sd");
    OSDisableInterrupts();
    exiPuts("Goodbye cruel world!\n");
    if(isReset) {
        //if launched from loader, make reset button return
        //tp the loader. to reset only the game, we can
        //use the controller (hold X+B+Start).
        //XXX make the controller method work...
        if(isLaunchedFromLoader) exit(0);
        else STM_RebootSystem();
    }
    else { //power off
        STM_ShutdownToStandby();
    }
}
