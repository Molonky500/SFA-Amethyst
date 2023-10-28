#include "main.h"

//extern DISC_INTERFACE __io_wiisd;

//save this for resets
void (*gameEntry)(void);

void bootGame() {
    //protect ARAM area from accidental accesses.
    //XXX why is SYS_PROTECTCHAN0 not defined?
    //and does "prevent all access" mean SYS_PROTECTNONE
    //or SYS_PROTECTRDWR?
    //SYS_ProtectRange(0,
    //    (void*)0x90000000, 0x01000000, SYS_PROTECTRDWR);
    //__MaskIrq(0xFFFFFFFF);
    //gameEntry = (void(*)(void))header->entryPoint;
    //memset((void*)0x90000000, 0, 0x01000000);
    gameEntry = (void(*)(void))0x803fa480;

    /*exiPrintf("WPAD_Shutdown...\n");
    WPAD_Shutdown();
    exiPrintf("BT Reset...\n");
    resetBluetooth();
    USB_Deinitialize();*/

    exiPrintf("Jumping to game entry point: %08X\r\n", gameEntry);
    //dumpMem(gameEntry, 256);
    //cache stuff is weird. changing this will cause random bugs.
    IRQ_Disable();
    iguanaSetBlueLed(true);
    LCDisable();
    L2GlobalInvalidate();
    L2Disable();
    //ICDisable();
    //DCDisable();
    iguanaSetBlueLed(false);

    //set up some bootinfo fields.
    OSBootInfo *boot = (OSBootInfo*)0x80000000;
    boot->diskId.gameName[0]      = 'G';
    boot->diskId.gameName[1]      = 'S';
    boot->diskId.gameName[2]      = 'A';
    boot->diskId.gameName[3]      = 'E';
    boot->diskId.company[0]       = '0';
    boot->diskId.company[1]       = '1';
    boot->diskId.diskNumber       = 0;
    boot->diskId.gameVersion      = 0;
    boot->diskId.streaming        = 1;
    boot->diskId.streamingBufSize = 0; //default
    boot->magic                   = 0x0d15ea5e; //magic
    boot->version                 = 1;
    boot->memorySize              = 0x01800000;
    //boot->consoleType             = 0x10000006;
    boot->arenaHi                 = 0x81800000;
    // *(u32*)0x800000F4 = 0; //dvdBI2

    *(u32*)0x80000040 = 0; //isDebuggerPresent
    *(u32*)0x80000044 = 0; //markedExceptions
        //(1 << 1) | //Machine Check
        //(1 << 2) | //DSI
        //(1 << 3) | //ISI
        //(1 << 6) ; //Alignment
    *(u32*)0x80000048 = 0x246DD8; //exceptionHookDest
    *(u32*)0x8000004C = 0; //exceptionHookLrSave

    /** Fill in BI2 */
    u32 *bi2 = (u32*)0x817e8240;
    *(u32*)0x800000F4 = (u32)bi2;
    memset(bi2, 0, 0x2000);
    bi2[0] = 0; //debug monitor size
    bi2[1] = 0x01800000; //pad spec
    bi2[2] = 0; //argv
    bi2[3] = 0; //argc (3 = enable TRK)
    bi2[6] = 1; //unknown
    bi2[7] = 1; //unknown
    bi2[8] = 1; //unknown

    //reset some hardware
    u32 RESET_BITS =
        (1 << 22) | //DSP (hangs system on boot if we do both DSP resets)
        //(1 << 21) | //VI1
        //(1 << 20) | //VI
        //(1 << 19) | //IOPI (reboots system)
        //(1 << 18) | //IOMEM
        //(1 << 17) | //IODI
        //(1 << 16) | //IOEXI (kills debug prints)
        (1 << 15) | //IOSI
        (1 << 14) | //AI_I2S3
        (1 << 13) | //GFX
        (1 << 12) | //GFX TCPE
        //(1 << 10) | //DI 2
        0;
    _ipcReg[0x194>>2] &= ~RESET_BITS; //clear = reset

    //_viReg[1] = 0; //unsure, but official code does it on reset
    _viReg[0] = 0; //reset
    // *(u16*)0xCC00500a = 1 | (1 << 11); //reset DSP
    udelay(500000);
    //_viReg[0] = 1; //un-reset
    _ipcReg[0x194>>2] |= RESET_BITS;
    // *(u16*)0xCC00500a = 0;
    //_ipcReg[0x70>>2] |= 1; //enable EXI (makes no difference)
    //udelay(1000000);
    __asm__ __volatile__ ("sync\n" "isync");
    gameEntry();
    //should never reach here...
    SET_SCREEN_SOLID_YUV(76, 84, 255); //red
    while(1);

    //tell the compiler we're not coming back from this one.
    __builtin_unreachable();
}
