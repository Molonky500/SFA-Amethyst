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

#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11

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
