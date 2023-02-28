#include "main.h"

char gameRootDir[512];

GameWiiInterface wiiIface;
u8 *loaderRebootCode = NULL;
bool isLaunchedFromLoader = false;
bool gIsSystemShuttingDown = false;
bool isReset = true; //is this shutdown really a reboot?

extern u32 __crt0stack, __crt0stack_end;
extern void *__ipcbufferLo, *__ipcbufferHi;
int save_argc=0;
char **save_argv = NULL;

void _initIos() {
    exiPuts("Init IOS...\n");
    SET_SCREEN_SOLID_YUV(255, 0, 148); //yellow
    //udelay(500000);
    initAlloc();
    exiPuts("loader2 alloc init OK\r\n");
    initLibc();
    exiPrintf("loader2 libc init OK, IPC buf %p - %p\r\n",
        __ipcbufferLo, __ipcbufferHi);
    //__lwp_wkspace_init(1*1024*1024);
    mtspr(920,(mfspr(920)&~0x40000000)); //DisableWriteGatherPipe();
    __IPC_ClntInit();
    __IOS_InitializeSubsystems();
    exiPrintf("IOS init OK.\n IPC Buf: 0x%8x - 0x%8x\n Arena1:  0x%8x - 0x%8x\n Arena2:  0x%8x - 0x%8x\n",
        __SYS_GetIPCBufferLo(), __SYS_GetIPCBufferHi(),
        *(u32*)0x8000310C, *(u32*)0x80003110,
        *(u32*)0x80003124, *(u32*)0x80003128);
}

int main(int argc, char **argv) {
    save_argc = argc;
    save_argv = argv;
    SET_SCREEN_SOLID_YUV(255, 128, 128); //white
    if(argc > 0) isLaunchedFromLoader = true;
    //else we're launched in Dolphin from command line or something
    //and there's no loader to exit to.

    //probe some GPIO stuff for debug
    _ipcReg[0x64>>2]  = 0xFFFFFFFF; //AHBPROT: access everything
    _ipcReg[0xFC>>2] |= 0x00FF0020; //allow PPC access
    _ipcReg[0xDC>>2]  = 0xFFFFFFFF; //enable GPIOs
    _ipcReg[0xC4>>2] |= 0x00FF0020; //set directions

    //init debug prints
    exiPrintInit();
    exiPuts("Hello world!\n");
    exiPrintf(" ---- loader2 start ---- \r\nstack: %08X - %08X\r\n",
        &__crt0stack_end, &__crt0stack);

    //copy text0 back to correct place
    //note Amethyst cuts off the first 0xA0 bytes because
    //they would overlap with IOS globals, so this isn't
    //the same address/length as normal
    static void *text0_dst = (void*)0x800031A0;
    static void *text0_src = (void*)0x80500000;
    static u32   text0_len = 0x2480;
    memcpy(text0_dst, text0_src, text0_len);
    DCFlushRange(text0_dst, text0_len);
    ICInvalidateRange(text0_dst, text0_len);
    exiPuts("Moved text0 OK\n");

    //no longer used
    //DolHeader header;
    //loadGameDol(&header);

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
    L2Enhance();

    exiPuts("boot game\r\n");
    bootGame(); //doesn't return
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

    //writeMemDump();
    exiPuts(isReset ? "*** SYSTEM REBOOTING\r\n" :
        "*** SYSTEM SHUTTING DOWN\r\n");
    u32 flags1 = *(u32*)0x803dcc80;
    u32 flags2 = *(u32*)0x803dcc84;
    exiPrintf("PI flags: %08X %08X\n", flags1, flags2);
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
