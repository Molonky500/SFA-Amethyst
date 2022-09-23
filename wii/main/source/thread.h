#define MAX_THREADS 128
#define THREAD_DEBUG 0

typedef void*(*OSThreadFunc)(void*);
typedef struct OSThread OSThread;

typedef struct {
    OSThreadFunc func;
    void *param;
    //used for debug, not used by the thread itself
    void *stack;
    u32 stackSize;
    OSThread *dolThread;
} HackThreadParam;

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
