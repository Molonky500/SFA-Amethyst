#include "main.h"

//extern DISC_INTERFACE __io_wiisd;

//save this for resets
void (*gameEntry)(void);

void jumpToGame() {
    exiPrintf("Jumping to game entry point: %08X\r\n", gameEntry);
    dumpMem(gameEntry, 64);
    IRQ_Disable();

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
    boot->consoleType             = 0x10000006;
    boot->arenaHi                 = 0x81800000;

    /*SET_SCREEN_SOLID_YUV(141, 191, 26); //light blue
    udelay(500000);
    SET_SCREEN_SOLID_YUV(255, 0, 148); //yellow
    udelay(500000);*/
    _viReg[1] = 0; //unsure, but official code does it on reset
    _viReg[0] = 0; //reset
    udelay(500000);
    _viReg[0] = 1; //un-reset
    //udelay(500000);
    gameEntry();
    //should never reach here...
    //SET_SCREEN_SOLID_YUV(76, 84, 255); //red
    while(1);

    //tell the compiler we're not coming back from this one.
    __builtin_unreachable();
}

void bootGame(DolHeader *header) {
    //protect ARAM area from accidental accesses
    //SYS_ProtectRange(SYS_PROTECTCHAN0,
    //    (void*)0x90000000, 0x01000000, SYS_PROTECTNONE);
    //__MaskIrq(0xFFFFFFFF);
    gameEntry = (void(*)(void))header->entryPoint;
    //gameEntry = (void(*)(void))0x803fa480;
    jumpToGame();
}
