#pragma once

extern "C" {
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
    #include <math.h>
    #include <debug.h>
    #include <sys/iosupport.h>
    #include <wiiuse/wpad.h>
};

#include "Gx.h"
#include "Texture.h"

//#define _ipcReg ((vu32*)0xCD800000)
#define SET_DEBUG_PORT(val) _ipcReg[0xC0>>2] = (_ipcReg[0xC0>>2] & ~0xFF0000) | ((val) << 16);
#define SET_DISC_LED(on) _ipcReg[0xC0>>2] = ((on) ? (_ipcReg[0xC0>>2] | 0x20) : (_ipcReg[0xC0>>2] & ~0x20))

extern "C" void* memalign(u32 align, size_t size);

#include "Font.h"

//dprint.c
void initDebugPrint();

//gx.c
extern Mtx gMtxView;
extern Mtx44 gMtxPerspective;

//main.c
extern std::filesystem::path gRootDir;
extern u32 gFrameCount;
extern GX::Font *gSystemFont;
uint32_t crc32b(const void *data_, uint32_t len);
