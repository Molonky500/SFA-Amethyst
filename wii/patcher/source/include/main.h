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
#include <wiiuse/wpad.h>

//#define _ipcReg ((vu32*)0xCD800000)
#define SET_DEBUG_PORT(val) _ipcReg[0xC0>>2] = (_ipcReg[0xC0>>2] & ~0xFF0000) | ((val) << 16);
#define SET_DISC_LED(on) _ipcReg[0xC0>>2] = ((on) ? (_ipcReg[0xC0>>2] | 0x20) : (_ipcReg[0xC0>>2] & ~0x20))

void* memalign(u32 align, size_t size);

typedef struct {
    union {
        struct { u8 r; u8 g; u8 b; u8 a; };
        u32 value;
    };
} Color4b;

//vertex format we use
typedef struct {
    s16 x, y;
    Color4b c;
    s16 s, t;
} AppVtx;

#include "font.h"

//gx.c
extern Mtx gMtxView;
extern Mtx44 gMtxPerspective;
int appGxInit();
void appGxFrameBegin();
void appGxFrameEnd();
void appGxGetScreenSize(u16 *width, u16 *height);
Color4b hsv2rgb(u8 h, u8 s, u8 v, u8 a);

//main.c
extern u32 gFrameCount;
uint32_t crc32b(const void *data_, uint32_t len);

}; //extern "C"

