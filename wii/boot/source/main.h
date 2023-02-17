#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <fat.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <wiiuse/wpad.h>

//1 = use custom hardware for debug print on real console
//0 = use "official" UART for Dolphin/real devkit
#define USE_CUSTOM_GECKO 1

#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11
//this address will later be used by the game as ARAM.
//that means the loader isn't using it at all and it
//won't go to waste storing a copy of the DOL.
//ARAM is 16MB and the DOL is < 4MB so this works out.
//addresses below this tend to get corrupted when we
//execute the second-stage loader on real hardware.
#define DOL_LOAD_ADDR 0x90C00000

typedef struct {
    u32 textOffset[DOL_NUM_TEXT_SECTIONS];
    u32 dataOffset[DOL_NUM_DATA_SECTIONS];
    u32 textAddr  [DOL_NUM_TEXT_SECTIONS];
    u32 dataAddr  [DOL_NUM_DATA_SECTIONS];
    u32 textSize  [DOL_NUM_TEXT_SECTIONS];
    u32 dataSize  [DOL_NUM_DATA_SECTIONS];
    u32 bssAddr, bssSize, entryPoint;
} DolHeader;

//exi.c
void exiPuts(const char *str);
void exiPrintf(const char *fmt, ...);
void exiPrintInit();

//main.c
extern bool gIsVideoInit;
