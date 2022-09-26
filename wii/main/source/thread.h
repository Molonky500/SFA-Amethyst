#define MAX_THREADS 128
#define THREAD_DEBUG 0

#define OS_THREAD_STATE_READY    0x1
#define OS_THREAD_STATE_RUNNING  0x2
#define OS_THREAD_STATE_WAITING  0x4
#define OS_THREAD_STATE_MORIBUND 0x8
#define OS_MESSAGE_NOBLOCK  0
#define OS_MESSAGE_BLOCK    1

typedef void*(*OSThreadFunc)(void*);
typedef struct OSThread OSThread;
typedef struct OSMessageQueue   OSMessageQueue;
typedef void*  OSMessage;

typedef struct {
    OSThread *dThread;
    bool allocatedStack;
} mapDolToLwpThread_t;
extern mapDolToLwpThread_t mapDolToLwpThread[MAX_THREADS];

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

struct OSAlarm
{
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

struct OSThreadQueue
{
    OSThread*  head;
    OSThread*  tail;
};

struct OSThreadLink
{
    OSThread*  next;
    OSThread*  prev;
};

struct OSMutexQueue
{
    OSMutex*   head;
    OSMutex*   tail;
};

struct OSMutexLink
{
    OSMutex*   next;
    OSMutex*   prev;
};

struct OSThread
{
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

struct OSMessageQueue
{
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
extern void (*__OSReschedule)(int);
extern void (*OSSleepThread)(OSThreadQueue* queue);
extern void (*OSWakeupThread)(OSThreadQueue* queue);
extern void (*OSInitThreadQueue)(OSThreadQueue* queue);
extern int (*OSDisableInterrupts)(void);
extern int (*OSEnableInterrupts)(void);
extern int (*OSRestoreInterrupts)(int);

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

extern s32 (*LWP_CreateThread_hook)(lwp_t *thethread,void* (*entry)(void *),void *arg,void *stackbase,u32 stack_size,u8 prio);
extern s32 (*LWP_SuspendThread_hook)(lwp_t thethread);
extern s32 (*LWP_ResumeThread_hook)(lwp_t thethread);
extern BOOL (*LWP_ThreadIsSuspended_hook)(lwp_t thethread);
extern lwp_t (*LWP_GetSelf_hook)(void);
extern void (*LWP_SetThreadPriority_hook)(lwp_t thethread,u32 prio);
extern void (*LWP_YieldThread_hook)(void);
extern void (*LWP_Reschedule_hook)(u32 prio);
extern s32 (*LWP_JoinThread_hook)(lwp_t thethread,void **value_ptr);
extern s32 (*LWP_InitQueue_hook)(lwpq_t *thequeue);
extern void (*LWP_CloseQueue_hook)(lwpq_t thequeue);
extern s32 (*LWP_ThreadSleep_hook)(lwpq_t thequeue);
extern void (*LWP_ThreadSignal_hook)(lwpq_t thequeue);
extern void (*LWP_ThreadBroadcast_hook)(lwpq_t thequeue);
extern s32 (*MQ_Init_hook)(mqbox_t *mqbox,u32 count);
extern void (*MQ_Close_hook)(mqbox_t mqbox);
extern BOOL (*MQ_Send_hook)(mqbox_t mqbox,mqmsg_t msg,u32 flags);
extern BOOL (*MQ_Jam_hook)(mqbox_t mqbox,mqmsg_t msg,u32 flags);
extern BOOL (*MQ_Receive_hook)(mqbox_t mqbox,mqmsg_t *msg,u32 flags);
