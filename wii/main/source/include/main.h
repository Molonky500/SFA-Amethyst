#include <stdio.h>
#include <stdlib.h>
//#include <fat.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include "gcc-macros.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define PANIC(msg) do { \
    exiPrintf(" *** ERROR *** %s:%d: %s\n", __FILE__, __LINE__, msg); \
    *(u32*)0 = 0; /* crash to get backtrace */ \
} while(0)
//XXX remove
#define HALT do { \
    exiPrintf(" *** ERROR *** HALT at %s:%d\n", __FILE__, __LINE__); \
    *(u32*)0 = 0; /* crash to get backtrace */ \
} while(0)

//this macro excludes 90xxxxxx (ARAM)
#define PTR_VALID(p) (((u32)(p) >= 0x80000000 && (u32)(p) <= 0x817FFFFF) || \
    ((u32)(p) >= 0x91000000 && (u32)(p) <= 0x93FFFFFF))

#define cntlzw(_val) ({u32 _rval; \
    __asm__ __volatile__ ("cntlzw %0, %1" : "=r"((_rval)) : "r"((_val))); _rval;})

#define RETURN_ADDRESS (__builtin_extract_return_addr(__builtin_return_address(0)))
#define ARGV_MAX 4096 //max argv total length
#define ARGC_MAX 16   //max arg count
extern char _argv_raw[ARGV_MAX];
extern char *_argc_raw[ARGC_MAX];
extern int _argc;

#define IPC_QUEUE_MAX 256
#define USB_ALIGN __attribute__ ((aligned(32)))

#include "asminline.h"
#include "lwpnode.h"
#include "printf.h"
#include "gctypes.h"
#include "gcbool.h"
#include "gcutil.h"
#include "processor.h"
#include "cache.h"
#include "regs.h"
#include "mem.h"
#include "malloc.h"
#include "clock.h"
#include "timesupp.h"
#include "irq.h"
#include "dvd.h"
#include "osbootinfo.h"
#include "dol.h"
#include "thread.h"
#include "lwp_heap.h"
#include "lwp_wkspace.h"
#include "sys_alarm.h"
#include "ipc.h"
#include "ios.h"
#include "disc_io.h"
#include "gameheap.h"
#include "gamefuncs.h"
#include "bte.h"
#include "conf.h"
#include "pad.h"
#include "gamecontrols.h"

int fatInitDefault();

//alloc.c
void dumpGameHeaps();
bool checkAddrInheap(void *addr, u32 len);
void* _my_sbrk_r(struct _reent *ptr, ptrdiff_t incr);

//audio.c
void ARStartDMA_hook(u32 cntH, void *mmaddr, u32 araddr, u32 cntL);
void AIInitDMA_hook(u32 start, uint length);
void AIStartDMA_hook();

//checkthread.c
extern OSThread checkThread;
void registerThreadForDebug(OSThread *thread, const char *name);
void initCheckThread();
void checkIntegrity();
void* checkThreadMain(void *param);

//debugprint.c
void osPrintHook(const char *fmt, ...);
void putHex(char *dst, u32 num);

//dol.c
void printDolHeader(DolHeader *header);
void loadDolFromMemory(DolHeader *header);

//dvd.c
extern volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];
extern OSMutex dvdMsgMutex;
void initDvdHack();
int sendToDvdThread(HackDvdMsg *msg);
int recvFromDvdThread(HackDvdMsg **msg, u32 flags);
int sendReadToDvdThread(DVDFileInfo *info, void *addr, uint length,
    int offset, DVDCallback callback, int prio, bool async);

//dvdhook.c
void __DVDFSInit_hook(void);
bool DVDOpen_hook(const char *path, DVDFileInfo *info);
int DVDRead_hook(DVDFileInfo *file, void *addr, uint size, uint offset);
int DVDCancelAsync_hook(DVDFileInfo *info, DVDCBCallback callback);
s32 DVDReadPrio_hook(DVDFileInfo* file, void* addr,
    s32 length, s32 offset, s32 prio);
bool DVDReadAsyncPrio_hook(DVDFileInfo *file, void *addr, int length,
    uint offset, DVDCallback callback, int prio);
BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
    u32 offset, DVDCallback callback);
BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
    DVDCBCallback callback);

//dvdthread.c
extern volatile bool dvdThreadReady;
extern OSThreadQueue dvdThreadQueue;
extern OSThread hackDvdThread;
extern OSMutex dvdFileInfoMutex;
extern OSMessageQueue hackDvdThreadMailIn, hackDvdThreadMailOut;
extern OSMessage hackDvdMailboxIn[DVD_MAX_MSGS], hackDvdMailboxOut[DVD_MAX_MSGS];
extern int dvdMsgsInHead, dvdMsgsInTail, dvdMsgsOutHead, dvdMsgsOutTail;
extern volatile HackDvdMsg dvdMsgsIn[DVD_MAX_MSGS], dvdMsgsOut[DVD_MAX_MSGS];
extern OSAlarm dvdThreadAlarm;
extern u32 dvdCmdId;
void dvdThreadAlarmCb(OSAlarm *alarm, OSContext *ctx);
void* hackDvdThreadMain(void *param);
HackDvdOpenFile* dvd_getFileByInfo(DVDFileInfo *info);
HackDvdOpenFile* dvd_getFileByHandle(FILE *file);
HackDvdOpenFile* dvd_addFile(DVDFileInfo *info, FILE *file, const char *path);
void dvd_removeFile(HackDvdOpenFile* file);

//exception.c
void OSExceptionInit_hook();
void exceptionHook(u32 *exc_gpr, int code);
void gameExceptionHook(int exceptionCode, OSContext *ctx,
    uint cause, void *addr);

//exi.c
void exiPuts(const char *str);
void exiPrintf(const char *fmt, ...);
void exiPrintInit();

//gameboot.c
extern void *acrIrq;
void bootGame(DolHeader *header);

//init.c
int init();

//ipc.c
void initIpc();
void ipcIrqHandler(int irqNo, OSContext *ctx);

//irq.c
extern vs32 irqHandlerDepth;
extern vs32 curIrqHandler;
extern vu32 lastIrqCause;
extern vu32 lastIrqCause2;
extern vu32 *gameIrqHandlers;
void gameExtIrqHandler_hook(int irqNo, OSContext *ctx);
void __OSInterruptInit_hook();
//void* __OSSetInterruptHandler_hook(int irq, void *handler);
//void* __OSGetInterruptHandler_hook(int irq);
void __OSMaskInterrupts_hook(u32 mask);
void __OSUnmaskInterrupts_hook(u32 mask);
void _irqPiError(int irq, OSContext *ctx);

//libc.c
void initLibc();

//main.c
extern char gameRootDir[512];

//malloc.c
extern u8 *heapCanaryTop, *heapCanaryBottom;
void initAlloc();
void checkAlloc();

//osfont.c
uint OSGetFontEncode_hook();
char* OSGetFontWidth_hook(char* string, s32* width);
char* OSGetFontTexel_hook(char* string, void* image, s32 pos,
    s32 stride, s32* width);

//patches.c
uint32_t hookBranch(uint32_t addr, void *target, bool isBl, bool forceTrampoline);
void doPatches();

//system.c
void* __SYS_GetIPCBufferLo(void);
void* __SYS_GetIPCBufferHi(void);

//system_asm.S
extern u32 get_r1();
extern u32 get_r2();
extern u32 get_r13();
extern void set_r1(u32);
extern void set_r2(u32);
extern void set_r13(u32);
extern u32 get_msr();
extern void set_msr(u32);
extern u32 get_dar();

//thread.c
void OSYieldThread(void);
void __OSPromoteThread(OSThread* thread, OSPriority priority);
void OSSetPeriodicAlarm(OSAlarm *alarm, OSTime tStart,
OSTime period, OSAlarmHandler handler);

//wiimote.c
void initWiimote();
void padUpdate_hook(void);
