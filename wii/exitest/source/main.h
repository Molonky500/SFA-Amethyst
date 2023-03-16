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

#define SPI_RXBUF_SIZE 128
//#define _ipcReg ((vu32*)0xCD800000)
#define SET_DEBUG_PORT(val) _ipcReg[0xC0>>2] = (_ipcReg[0xC0>>2] & ~0xFF0000) | ((val) << 16);
#define SET_DISC_LED(on) _ipcReg[0xC0>>2] = ((on) ? (_ipcReg[0xC0>>2] | 0x20) : (_ipcReg[0xC0>>2] & ~0x20))

//main.c
extern bool gIsVideoInit;
