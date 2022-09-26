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

//this macro excludes 90xxxxxx (ARAM) and 93xxxxxx (IOS)
#define PTR_VALID(p) (((p) >= 0x80000000 && (p) <= 0x817FFFFF) || \
    ((p) >= 0x91000000 && (p) <= 0x92FFFFFF))

#define __OSBusClock  (*(u32*)0x800000F8)
#define __OSCoreClock (*(u32*)0x800000FC)
#define OS_BUS_CLOCK        __OSBusClock
#define OS_CORE_CLOCK       __OSCoreClock
#define OS_TIMER_CLOCK      (OS_BUS_CLOCK/4)
#define OSTicksToCycles( ticks )        (((ticks) * ((OS_CORE_CLOCK * 2) / OS_TIMER_CLOCK)) / 2)
#define OSTicksToSeconds( ticks )       ((ticks) / OS_TIMER_CLOCK)
#define OSTicksToMilliseconds( ticks )  ((ticks) / (OS_TIMER_CLOCK / 1000))
#define OSTicksToMicroseconds( ticks )  (((ticks) * 8) / (OS_TIMER_CLOCK / 125000))
#define OSTicksToNanoseconds( ticks )   (((ticks) * 8000) / (OS_TIMER_CLOCK / 125000))
#define OSSecondsToTicks( sec )         ((sec)  * OS_TIMER_CLOCK)
#define OSMillisecondsToTicks( msec )   ((msec) * (OS_TIMER_CLOCK / 1000))
#define OSMicrosecondsToTicks( usec )   (((usec) * (OS_TIMER_CLOCK / 125000)) / 8)
#define OSNanosecondsToTicks( nsec )    (((nsec) * (OS_TIMER_CLOCK / 125000)) / 8000)

#include "thread.h"
#include "dvd.h"

//alloc.c
void* alloc_hook(u32 size, u32 tag, const char *name);
void free_hook(void *addr);
void alloc_init();
bool checkAddrInheap(void *addr, u32 len);

//audio.c
void ARStartDMA_hook(u32 cntH, void *mmaddr, u32 araddr, u32 cntL);
void AIInitDMA_hook(u32 start, uint length);

//debugprint.c
void osPrintHook(const char *fmt, ...);
void DBPrintf_hook(const char *fmt, ...);
void dspDebugPrint_hook(const char *fmt, ...);

//dol.c
void printDolHeader(DolHeader *header);
void loadDolFromMemory(DolHeader *header);

//dvd.c
extern volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];
void initDvdHack();

//dvdhook.c
void __DVDFSInit_hook(void);
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

//dvdio.c
int sendToDvdThread(HackDvdMsg *msg);
int recvFromDvdThread(HackDvdMsg **msg, u32 flags);
int sendReadToDvdThread(DVDFileInfo *info, void *addr, uint length,
    int offset, DVDCallback callback, int prio, bool async);

//dvdthread.c
extern volatile bool dvdThreadReady;
extern OSThreadQueue dvdThreadQueue;
extern OSThread hackDvdThread;
extern OSMessageQueue hackDvdThreadMailIn, hackDvdThreadMailOut;
extern int dvdMsgsInHead, dvdMsgsInTail, dvdMsgsOutHead, dvdMsgsOutTail;
extern volatile HackDvdMsg dvdMsgsIn[DVD_MAX_MSGS], dvdMsgsOut[DVD_MAX_MSGS];
extern OSAlarm dvdThreadAlarm;
extern u32 dvdCmdId;
void dvdThreadAlarmCb(OSAlarm *alarm, OSContext *ctx);
void* hackDvdThreadMain(void *param);
HackDvdOpenFile* dvd_getFileByInfo(DVDFileInfo *info);
HackDvdOpenFile* dvd_getFileByHandle(FILE *file);
HackDvdOpenFile* dvd_addFile(DVDFileInfo *info, FILE *file);
void dvd_removeFile(HackDvdOpenFile* file);

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
void OSExceptionInit_hook();

//main.c
extern char gameRootDir[512];

//osalarm.c
extern void (*OSCreateAlarm)(OSAlarm *alarm);
extern void (*OSSetAlarm)(OSAlarm*       alarm,
                OSTime         tick,
                OSAlarmHandler handler);

//osfont.c
uint OSGetFontEncode_hook();
char* OSGetFontWidth_hook(char* string, s32* width);
char* OSGetFontTexel_hook(char* string, void* image, s32 pos,
    s32 stride, s32* width);

//patches.c
uint32_t hookBranch(uint32_t addr, void *target, int isBl);
void doPatches();

//thread.c
void OSYieldThread(void);
void initThreads();
s32 LWP_CreateThread_dol(lwp_t *thethread,
    void* (*entry)(void *),void *arg,void *stackbase,
    u32 stack_size,u8 prio);
s32 LWP_SuspendThread_dol(lwp_t thethread);
s32 LWP_ResumeThread_dol(lwp_t thethread);
BOOL LWP_ThreadIsSuspended_dol(lwp_t thethread);
lwp_t LWP_GetSelf_dol(void);
void LWP_SetThreadPriority_dol(lwp_t thethread,u32 prio);
void LWP_YieldThread_dol(void);
void LWP_Reschedule_dol(u32 prio);
s32 LWP_JoinThread_dol(lwp_t thethread,void **value_ptr);
s32 LWP_InitQueue_dol(lwpq_t *thequeue);
void LWP_CloseQueue_dol(lwpq_t thequeue);
s32 LWP_ThreadSleep_dol(lwpq_t thequeue);
void LWP_ThreadSignal_dol(lwpq_t thequeue);
void LWP_ThreadBroadcast_dol(lwpq_t thequeue);

//threadmsg.c
s32 MQ_Init_dol(mqbox_t *mqbox,u32 count);
void MQ_Close_dol(mqbox_t mqbox);
BOOL MQ_Send_dol(mqbox_t mqbox,mqmsg_t msg,u32 flags);
BOOL MQ_Jam_dol(mqbox_t mqbox,mqmsg_t msg,u32 flags);
BOOL MQ_Receive_dol(mqbox_t mqbox,mqmsg_t *msg,u32 flags);

//XXX mutexes
