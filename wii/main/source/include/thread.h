#ifndef __THREAD_H__
#define __THREAD_H__

#define MAX_THREADS 128
#define THREAD_DEBUG 0

#define OS_THREAD_STATE_READY    0x1
#define OS_THREAD_STATE_RUNNING  0x2
#define OS_THREAD_STATE_WAITING  0x4
#define OS_THREAD_STATE_MORIBUND 0x8
#define OS_MESSAGE_NOBLOCK  0
#define OS_MESSAGE_BLOCK    1

#include "gcbool.h"
#include "gctypes.h"

#define _osDisableThreadSwitching (*(u32*)0x803dde8c)

typedef void*(*OSThreadFunc)(void*);
typedef struct OSThread OSThread;
typedef struct OSMessageQueue   OSMessageQueue;
typedef void*  OSMessage;

typedef struct {
    u32	gpr[32];
    u32	cr;
    u32	lr;
    u32	ctr;
    u32	xer;
    f64	fpr[32];
    u64	fpscr;
    u32	srr0;
    u32	srr1;
    u16	mode;
    u16	state; //1:has FPU, 2:has exception
    u32	gqr[8];
    u32	psf_padding;
    f64	psf[32];
} OSContext;

typedef struct OSAlarm  OSAlarm;
typedef void          (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

#define OS_THREAD_SPECIFIC_MAX  2

//typedef struct OSThread         OSThread;
typedef struct OSThreadQueue    OSThreadQueue;
typedef struct OSThreadLink     OSThreadLink;
typedef s32                     OSPriority;     //  0 highest, 31 lowest

typedef struct OSMutex          OSMutex;
typedef struct OSMutexQueue     OSMutexQueue;
typedef struct OSMutexLink      OSMutexLink;
typedef struct OSCond           OSCond;

typedef void                  (*OSIdleFunction)(void* param);
typedef void                  (*OSSwitchThreadCallback)(OSThread* from, OSThread* to);
typedef s64         OSTime;
typedef u32         OSTick;

struct OSAlarm {
    OSAlarmHandler  handler;
    u32             tag;
    OSTime          fire;
    OSAlarm*        prev;
    OSAlarm*        next;

    // Periodic alarm
    OSTime          period;
    OSTime          start;

    // User data
    void*           userData;
};

struct OSThreadQueue {
    OSThread*  head;
    OSThread*  tail;
};

struct OSThreadLink {
    OSThread*  next;
    OSThread*  prev;
};

struct OSMutexQueue {
    OSMutex*   head;
    OSMutex*   tail;
};

struct OSMutexLink {
    OSMutex*   next;
    OSMutex*   prev;
};

struct OSMutex {
    OSThreadQueue   queue;
    OSThread*       thread; // the current owner
    s32             count;  // lock count
    OSMutexLink     link;   // for OSThread.queueMutex
};

struct OSCond {
    OSThreadQueue   queue;
};

struct OSThread {
    OSContext       context;    // register context

    u16             state;      // OS_THREAD_STATE_*
    u16             attr;       // OS_THREAD_ATTR_*
    s32             suspend;    // suspended if the count is greater than zero
    OSPriority      priority;   // effective scheduling priority
    OSPriority      base;       // base scheduling priority
    void*           val;        // exit value

    OSThreadQueue*  queue;      // queue thread is on
    OSThreadLink    link;       // queue link

    OSThreadQueue   queueJoin;  // list of threads waiting for termination (join)

    OSMutex*        mutex;      // mutex trying to lock
    OSMutexQueue    queueMutex; // list of mutexes owned

    OSThreadLink    linkActive; // link of all threads for debugging

    u8*             stackBase;  // the thread's designated stack (high address)
    u32*            stackEnd;   // last word of stack (low address)

    s32             error;
    void*           specific[OS_THREAD_SPECIFIC_MAX];   // thread specific data
};

struct OSMessageQueue {
    OSThreadQueue   queueSend;
    OSThreadQueue   queueReceive;
    OSMessage*      msgArray;
    s32             msgCount;
    s32             firstIndex;
    s32             usedCount;
};

//thread.c
extern BOOL (*OSCreateThread)(
    OSThread*  thread,
    void*    (*func)(void*),
    void*      param,
    void*      stackBase,
    u32        stackSize,
    OSPriority priority,
    u16        attribute);
extern s32 (*OSResumeThread)(OSThread* thread);
extern s32 (*OSSuspendThread)(OSThread* thread);
extern OSThread* (*OSGetCurrentThread)(void);
extern BOOL (*OSSetThreadPriority)(OSThread* thread, OSPriority priority);
extern void (*SelectThread)(BOOL);
extern void (*__OSReschedule)(void);
extern void (*OSSleepThread)(OSThreadQueue* queue);
extern void (*OSWakeupThread)(OSThreadQueue* queue);
extern void (*OSInitThreadQueue)(OSThreadQueue* queue);
extern int (*OSDisableInterrupts)(void);
extern int (*OSEnableInterrupts)(void);
extern int (*OSRestoreInterrupts)(int);
void OSYieldThread(void);
void __OSPromoteThread(OSThread* thread, OSPriority priority);

//threadmsg.c
extern void (*OSInitMessageQueue)(
    OSMessageQueue* mq,
    OSMessage*      msgArray,
    s32             msgCount);
extern BOOL (*OSSendMessage)(
    OSMessageQueue* mq,
    OSMessage       msg,
    s32             flags);
/*extern BOOL (*OSJamMessage)(
    OSMessageQueue* mq,
    OSMessage       msg,
    s32             flags);*/
extern BOOL (*OSReceiveMessage)(
    OSMessageQueue* mq,
    OSMessage*      msg,
    s32             flags);
extern int (*__OSGetEffectivePriority)(OSThread*);
extern OSThread* (*SetEffectivePriority)(OSThread*, int);

//these don't seem to be in the game so I've added them
//to this program.
void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
bool OSTryLockMutex(OSMutex* mutex);
void OSInitCond(OSCond* cond);
void OSWaitCond(OSCond* cond, OSMutex* mutex);
void OSSignalCond(OSCond* cond);

#endif //__THREAD_H__
