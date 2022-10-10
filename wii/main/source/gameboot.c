#include "main.h"

//extern DISC_INTERFACE __io_wiisd;

//save this for resets
void (*gameEntry)(void);

void jumpToGame() {
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

    SET_SCREEN_SOLID_YUV(141, 191, 26); //light blue
    gameEntry();
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
    jumpToGame();
}
