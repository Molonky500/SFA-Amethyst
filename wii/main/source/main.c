#include "main.h"

//magic string. the stage 1 loader will overwrite this string
//in memory with the actual root directory.
char gameRootDir[512] = "*** GAME ROOT DIR ***";

GameWiiInterface wiiIface;
u8 loaderRebootCode[6144];
bool isLaunchedFromLoader = false;
bool gIsSystemShuttingDown = false;
bool isReset = true; //is this shutdown really a reboot?

extern u32 __crt0stack, __crt0stack_end;

int main(int argc, char **argv) {
    SET_SCREEN_SOLID_YUV(255, 128, 128); //white
    if(argc > 0) isLaunchedFromLoader = true;
    //else we're launched in Dolphin from command line or something
    //and there's no loader to exit to.

    exiPrintInit();
    exiPrintf(" ---- loader2 start ---- \r\nstack: %08X - %08X\r\n",
        &__crt0stack_end, &__crt0stack);

    memcpy(loaderRebootCode, (void*)0x80001800, 6144);
    DolHeader *header = (DolHeader*)DOL_LOAD_ADDR;
    loadDolFromMemory(header);

    exiPrintf("game loader start; argc=%d\r\n", argc);
    for(int i=0; i<argc; i++) {
        exiPrintf("argv[%d] = \"%s\"\r\n", i, argv[i]);
    }

    u32 version = *(u32*)0x80003140;
    exiPrintf("Running on IOS%d v%d.%d\r\n",
        version >> 16, (version >> 8) & 0xFF, version & 0xFF);

    exiPuts("apply patches\r\n");
    doPatches();

    wiiIface.magic = WII_IFACE_MAGIC;
    WII_IFACE_PTR = &wiiIface;

    STM_RegisterEventHandler(MyStmHandler);

    exiPuts("boot game\r\n");
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
            if(*(u8*)0x803dcca6 == 0) {
                // *(float*)0x803dcb00 = 4.0f; //reset fadeout timer
                //we get FPU Unavailable exception...
                // *(u32*)0x803dcb00 = 0x40800000;
                *(u8*)0x803dcca6 = 1; //shouldReset
                *(u8*)0x803dca3e = 1; //shouldResetNextFrame
                exiPuts("Told game to reboot!\r\n");
            }
            //OSRebootHook();
            break;

        default:
            exiPrintf("Unknown STM event %08X\r\n", event);
    }
}

void OSRebootHook() {
    gIsSystemShuttingDown = true;
    exiPuts(isReset ? "*** SYSTEM REBOOTING\r\n" :
        "*** SYSTEM SHUTTING DOWN\r\n");
    OSCancelThread(&hackDvdThread);
    SET_SCREEN_SOLID_YUV(106, 139, 94); //teal
    udelay(500000);
    WPAD_Shutdown();
    fatUnmount("sd");
    OSDisableInterrupts();
    exiPuts("Goodbye cruel world!\r\n");
    if(isReset) {
        //if launched from loader, make reset button return
        //to the loader. to reset only the game, we can
        //use the controller (hold X+B+Start).
        //XXX make the controller method work...
        if(isLaunchedFromLoader) exit(0);
        else STM_RebootSystem();
    }
    else { //power off
        STM_ShutdownToStandby();
    }
}
