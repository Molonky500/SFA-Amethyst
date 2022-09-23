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

#define PACKED __attribute__(( packed ))
#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11

#define PI_INSTR        *(vu32*)0xcc003000
#define PI_INMTR        *(vu32*)0xcc003004
#define AR_DMA_MMADDR_H *(vu16*)0xcc005020
#define AR_DMA_MMADDR_L *(vu16*)0xcc005022
#define AR_DMA_ARADDR_H *(vu16*)0xcc005024
#define AR_DMA_ARADDR_L *(vu16*)0xcc005026
#define AR_DMA_CNT_H    *(vu16*)0xcc005028
#define AR_DMA_CNT_L    *(vu16*)0xcc00502a
#define AR_DMA_CNT_LEFT *(vu16*)0xCC00503a
#define AI_DMA_START_HI *(vu16*)0xCC005030
#define AI_DMA_START_LO *(vu16*)0xCC005032
#define AI_DMA_LENGTH   *(vu16*)0xCC005036

typedef struct {
    u32 textOffset[DOL_NUM_TEXT_SECTIONS];
    u32 dataOffset[DOL_NUM_DATA_SECTIONS];
    u32 textAddr  [DOL_NUM_TEXT_SECTIONS];
    u32 dataAddr  [DOL_NUM_DATA_SECTIONS];
    u32 textSize  [DOL_NUM_TEXT_SECTIONS];
    u32 dataSize  [DOL_NUM_DATA_SECTIONS];
    u32 bssAddr, bssSize, entryPoint;
} DolHeader;

#include "dvd.h"
#include "thread.h"

//alloc.c
void* alloc_hook(u32 size, u32 tag, const char *name);
void free_hook(void *addr);
void alloc_init();
bool checkAddrInheap(void *addr, u32 len);

//audio.c
void ARStartDMA_hook(u32 cntH, void *mmaddr, u32 araddr, u32 cntL);
void AIInitDMA_hook(u32 start, uint length);

//dol.c
void printDolHeader(DolHeader *header);
void loadDol(FILE *dol, DolHeader *header);

//dvd.c
void initDvdHack();
bool DVDOpen_hook(const char *path, DVDFileInfo *info);
int DVDRead_hook(DVDFileInfo *file, void *addr, uint size, uint offset);
int DVDClose_hook(DVDFileInfo *file);
s32 DVDReadPrio_hook(DVDFileInfo* file, void* addr,
    s32 length, s32 offset, s32 prio);
bool DVDReadAsyncPrio_hook(DVDFileInfo *file, void *addr, int length,
    uint offset, DVDCallback callback, int prio);
BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
    u32 offset, DVDCallback callback);
BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
    DVDCBCallback callback);

//exceptionRaw.s
void _raw_exceptionHook_Reset();
void _raw_exceptionHook_MachineCheck();
void _raw_exceptionHook_DSI();
void _raw_exceptionHook_ISI();
void _raw_exceptionHook_External();
void _raw_exceptionHook_Alignment();
void _raw_exceptionHook_Program();
void _raw_exceptionHook_FpUnavailable();
void _raw_exceptionHook_Decrementer();
void _raw_exceptionHook_Syscall();
void _raw_exceptionHook_Trace();
void _raw_exceptionHook_PerfMon();
void _raw_exceptionHook_IABR();
//void _raw_exceptionHook_Thermal();
void _raw_decrementerHook();
//we just need the address of these symbols
extern u32 _raw_exceptionHook_External2;
extern u32 _raw_exceptionHook_External_END;
extern u32 _raw_exceptionHook_FpUnavailable2;
extern u32 _raw_exceptionHook_FpUnavailable_END;
//XXX move these
extern u32 get_r1();
extern u32 get_r2();
extern u32 get_r13();
extern void set_r1(u32);
extern void set_r2(u32);
extern void set_r13(u32);
extern u32 get_msr();
extern void set_msr(u32);

//exception.c
void switchToGame();
void switchToOgc();
void putHex(char *dst, u32 num);
void exceptionHook(u32 *exc_gpr, int code);

//exi.c
void exiPuts(const char *str);
void exiPrintf(const char *fmt, ...);
void exiPrintInit();

//gameboot.c
void bootGame();

//init.c
int init();

//irq.c
void disableIrq();
void enableIrq();
void setupIrqHandler();
void gameIrqHandler(u32 irq, void *ctx);

//main.c
extern char gameRootDir[512];

//patches.c
uint32_t hookBranch(uint32_t addr, void *target, int isBl);
void doPatches(DolHeader *header);

//thread.c
void initThreads();
int OSCreateThread_hook(OSThread *thread, void *func, void *param,
void *stack2, u32 stackSize, OSPriority priority, u16 attr);
void OSCancelThread_hook(OSThread *thread);
void OSResumeThread_hook(OSThread *thread);
void OSSuspendThread_hook(OSThread *thread);
void OSSleepThread_hook(OSThreadQueue *queue);
void OSWakeupThread_hook(OSThreadQueue *queue);
OSThread* OSGetCurrentThread_hook();
void OSInitThreadQueue_hook(OSThreadQueue *queue);
void OSInitMessageQueue_hook(void *queue, u32 *msgArray, s32 msgCount);
BOOL OSSendMessage_hook(void *queue, u32 msg, u32 flags);
BOOL OSReceiveMessage_hook(void *queue, u32 *msg, u32 flags);
u32 _gameMaskInterrupts(u32 flags);
u32 _gameUnmaskInterrupts(u32 flags);
u32 OSMaskInterrupts_hook(u32 flags);
u32 OSUnmaskInterrupts_hook(u32 flags);
u32 OSDisableInterrupts_hook();
u32 OSEnableInterrupts_hook();
void OSRestoreInterrupts_hook(u32 level);
void OSSwitchThreadOrSomeShit_hook(int unused);
