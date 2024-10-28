#pragma once

extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <gccore.h>
    #include <fat.h>
    #include <di/di.h>
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
    void* memalign(u32 align, size_t size);
};

//#define _ipcReg ((vu32*)0xCD800000)
#define SET_DEBUG_PORT(val) _ipcReg[0xC0>>2] = (_ipcReg[0xC0>>2] & ~0xFF0000) | ((val) << 16);
#define SET_DISC_LED(on) _ipcReg[0xC0>>2] = ((on) ? (_ipcReg[0xC0>>2] | 0x20) : (_ipcReg[0xC0>>2] & ~0x20))

class App;
extern App *gApp;
#include "App.h"

//dprint.c
void initDebugPrint();

//gx.c
extern Mtx gMtxView;
extern Mtx44 gMtxPerspective;
