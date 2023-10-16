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

#define DMA_BUF_SIZE 4096
#define RX_BUF_SIZE 128
//#define _ipcReg ((vu32*)0xCD800000)
#define SET_DEBUG_PORT(val) _ipcReg[0xC0>>2] = (_ipcReg[0xC0>>2] & ~0xFF0000) | ((val) << 16);
#define SET_DISC_LED(on) _ipcReg[0xC0>>2] = ((on) ? (_ipcReg[0xC0>>2] | 0x20) : (_ipcReg[0xC0>>2] & ~0x20))

enum DataMode {
    HELLO,
    TEXT,
    PAYLOAD,
    ZERO,
    ONE,
    ZERO_ONE,
    ONE_ZERO,
    RANDOM,
    NUM_DATA_MODES
};

enum TestMode {
    TEST_READ,
    TEST_WRITE,
    TEST_BOTH,
    NUM_TEST_MODES
};

//main.c
extern bool gIsVideoInit;
extern s8  speed;
extern u8  payload[4];
extern s32 dataMode;
extern s32 testMode;
extern u8 debugPort;
extern bool autoWrite;
extern bool useDma;
extern bool useBulk;
extern u32  writeCount;
void quit(const char *msg);
void doWrite();
void doRead();
void doExec();

//menu.c
void drawMenu();
void runMenu();
