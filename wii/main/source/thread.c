#include "main.h"

HackThreadParam threadParam[MAX_THREADS];

struct {
    OSThread *dolphinThread;
    OSThreadQueue *dolphinQueue;
    lwp_t lwpThread;
} threadMap[MAX_THREADS];

struct {
    OSThreadQueue *dolphinQueue;
    lwpq_t lwpQueue;
} queueMap[MAX_THREADS];

struct {
    void *dolphinQueue;
    mqbox_t mQueue;
} msgQueueMap[MAX_THREADS];

void dumpThreads() {
    for(int i=0; i<MAX_THREADS; i++) {
        exiPrintf("Thread[%3d] %08X %08X %08X\n", i,
            threadMap[i].dolphinThread,
            threadMap[i].dolphinQueue,
            threadMap[i].lwpThread);
    }
    for(int i=0; i<MAX_THREADS; i++) {
        exiPrintf("Queue[%3d] %08X %08X\n", i,
            queueMap[i].dolphinQueue,
            queueMap[i].lwpQueue);
    }
    for(int i=0; i<MAX_THREADS; i++) {
        exiPrintf("MQueue[%3d] %08X %08X\n", i,
            msgQueueMap[i].dolphinQueue,
            msgQueueMap[i].mQueue);
    }
}

void addDolphinThread(OSThread *thread, lwp_t lwpThread) {
    exiPrintf("addDolphinThread(%08X, %08X) q=%08X\n",
        thread, lwpThread, thread->queue);
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(threadMap[i].dolphinThread == thread) {
            IRQ_Restore(irq);
            exiPrintf("addDolphinThread(%08X, %08X) already added (%d)\n",
                thread, lwpThread, i);
            return;
        }
        if(threadMap[i].dolphinThread == NULL) {
            threadMap[i].dolphinThread = thread;
            threadMap[i].dolphinQueue = thread->queue;
            threadMap[i].lwpThread = lwpThread;
            IRQ_Restore(irq);
            exiPrintf("addDolphinThread(%08X, %08X) OK (%d)\n",
                thread, lwpThread, i);
            return;
        }
    }
    IRQ_Restore(irq);
    exiPuts(" *** ERROR *** Too many threads!\n");
}

OSThread* getDolphinThreadForLwp(lwp_t lwpThread) {
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(threadMap[i].lwpThread == lwpThread) {
            IRQ_Restore(irq);
            return threadMap[i].dolphinThread;
        }
    }
    exiPrintf(" *** ERROR *** Can't find lwpthread %08X!\n", (u32)lwpThread);
    IRQ_Restore(irq);
    return NULL;
}
lwp_t getDolphinThread(OSThread *thread) {
    if(!thread) {
        exiPuts(" *** ERROR *** getDolphinThread(NULL)\n");
        return 0;
    }
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(threadMap[i].dolphinThread == thread) {
            IRQ_Restore(irq);
            return threadMap[i].lwpThread;
        }
    }

    IRQ_Restore(irq);
    exiPrintf(" *** ERROR *** Can't find thread %08X!\n", (u32)thread);
    dumpThreads();
    while(1);
    return 0;
}
lwp_t getDolphinThreadByQueue(OSThreadQueue *queue) {
    if(!queue) {
        exiPuts(" *** ERROR *** getDolphinThreadByQueue(NULL)\n");
        return 0;
    }
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(threadMap[i].dolphinQueue == queue) {
            IRQ_Restore(irq);
            return threadMap[i].lwpThread;
        }
    }

    IRQ_Restore(irq);
    exiPrintf(" *** ERROR *** Can't find dolqueue %08X!\n", (u32)queue);
    while(1);
    return 0;
}

lwpq_t* getQueue(OSThreadQueue *queue) {
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(queueMap[i].dolphinQueue == queue) {
            IRQ_Restore(irq);
            return &queueMap[i].lwpQueue;
        }
        else if(queueMap[i].dolphinQueue == NULL) {
            IRQ_Restore(irq);
            queueMap[i].dolphinQueue = queue;
            LWP_InitQueue(&queueMap[i].lwpQueue);
            return &queueMap[i].lwpQueue;
        }
    }
    IRQ_Restore(irq);
    exiPrintf(" *** ERROR *** Can't find queue %08X!\n", (u32)queue);
    //while(1);
    return NULL;
}

mqbox_t* addDolphinMsgQueue(void *queue, s32 msgCount) {
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(msgQueueMap[i].dolphinQueue == queue) {
            IRQ_Restore(irq);
            return &msgQueueMap[i].mQueue;
        }
        if(msgQueueMap[i].dolphinQueue == NULL) {
            msgQueueMap[i].dolphinQueue = queue;
            int r = MQ_Init(&msgQueueMap[i].mQueue, msgCount);
            if(r < 0) {
                exiPrintf(" *** ERROR *** Error creating msgqueue: %d\n", r);
            }
            IRQ_Restore(irq);
            return &msgQueueMap[i].mQueue;
        }
    }
    IRQ_Restore(irq);
    exiPuts(" *** ERROR *** Too many message queues!\n");
    return NULL;
}

mqbox_t* getMsgQueue(void *dolphinQueue) {
    u32 irq = IRQ_Disable();
    for(int i=0; i<MAX_THREADS; i++) {
        if(msgQueueMap[i].dolphinQueue == dolphinQueue) {
            IRQ_Restore(irq);
            return &msgQueueMap[i].mQueue;
        }
    }
    IRQ_Restore(irq);
    exiPrintf(" *** ERROR *** Can't find msgqueue %08X!\n", (u32)dolphinQueue);
    //dumpThreads();
    //while(1);
    return NULL;
}

lwp_t checkThread;
void* checkThreadMain(void *param) {
    exiPuts("Check-thread online\n");
    while(1) {
        for(int i=0; i<MAX_THREADS; i++) {
            if(threadParam[i].func) {
                u32 *stack = (u32*)threadParam[i].stack;
                if(stack[-threadParam[i].stackSize/4] != 0xDEADBABE) {
                    exiPrintf(" *** ERROR *** STACK CORRUPTION ON THREAD %08X\n",
                        threadParam[i].dolThread);
                    IRQ_Disable();
                    while(1);
                }
            }
        }
        LWP_YieldThread();
    }
    return NULL;
}

void initThreads() {
    memset(threadMap, 0, sizeof(threadMap));
    memset(queueMap, 0, sizeof(queueMap));
    memset(threadParam, 0, sizeof(threadParam));
    addDolphinThread((OSThread*)0x803ad848, LWP_GetSelf());

    //the game's main thread priority is 16, which is halfway
    //between lowest and highest, so do the same.
    LWP_SetThreadPriority(LWP_GetSelf(), 64);

    //set check-thread to low priority
    //LWP_CreateThread(
    //    &checkThread, checkThreadMain, NULL, NULL, 0, 1);
    exiPrintf("threads init (self=%08X)\n", (u32)LWP_GetSelf());
}

void* hackThreadMain(HackThreadParam *param) {
    //OSCreateThread spawns the thread suspended.
    //LWP_CreateThread doesn't have an option for this.
    //solution: have the new thread immediately suspend
    //itself.
    LWP_SuspendThread(LWP_GetSelf());
    switchToGame();
    return param->func(param->param);
}

int OSCreateThread_hook(OSThread *thread, void *func, void *param,
void *stack2, u32 stackSize, OSPriority priority, u16 attr) {
    /* Creates a new thread. The created thread is initially
     * suspended and must be made runnable by a call to
     * OSResumeThread().
     * This function takes the base (high address) of the stack
     * and the size of the stack so that it can write a magic
     * word into the last word of the stack.
     * OSCheckActiveThreads() will ensure that stack overflow
     * has not occurred. If you are encountering strange
     * behavior in your program, be sure to check that the
     * "stackEnd" field of your thread structures point to a
     * word with the OS_THREAD_STACK_MAGIC value.
     * Note: In order to match Nintendo 64 (N64) thread
     * semantics, choose OS_THREAD_ATTR_DETACH for the value of
     * attribute. This will ensure that the thread control
     * block is released as soon as the thread finishes
     * execution. If no attribute is chosen (i.e., 0 value for
     * attribute), the thread control block will remain in use
     * by the operating system until another thread joining
     * this thread (via OSJoinThread()) finally runs.
     * The joining thread thus receives the return value of
     * the terminating thread. This is also useful for
     * debugging, as the context of the thread can be
     * analyzed in the debugger after it has terminated.
     * A thread can also have the "detached" attribute set by
     * OSDetachThread().
     */
    //find a free slot
    HackThreadParam *hackParam = NULL;
    for(int i=0; i<MAX_THREADS; i++) {
        if(threadParam[i].func == NULL) {
            hackParam = &threadParam[i];
            break;
        }
    }
    if(!hackParam) {
        exiPuts(" *** ERROR *** Can't create more threads\n");
        return false;
    }
    hackParam->func  = (OSThreadFunc)func;
    hackParam->param = param;
    hackParam->stack = stack2;
    hackParam->stackSize = stackSize;
    hackParam->dolThread = thread;

    //call original OSCreateThread
    int (*OSCreateThread)(OSThread *thread, void *func, void *param,
    void *stack2, u32 stackSize, OSPriority priority, u16 attr) =
    (int(*)(OSThread *thread, void *func, void *param,
    void *stack2, u32 stackSize, OSPriority priority, u16 attr))0x802462a8;
    //int ok = OSCreateThread(thread, func, param, stack2, stackSize, priority, attr);
    int ok = 1;
    switchToOgc();
    lwp_t l;
    //stack2 = NULL; //let it allocate a new stack
    //NOTE: that causes false-positive corruption detection
    //so need to disable check-thread too.
    //LWP does also write this at the top/bottom of the stack,
    //but we don't know what the stack address will be to give
    //to the check-thread.

    stack2 -= stackSize;
    u32 *stack = (u32*)stack2;
    stack[-stackSize/4] = 0xDEADBABE;

    //LWP priority has higher number = higher priority
    //Dolphin has the other way around lol
    int prio = MIN(100, 127 - (priority * 4));
    s32 r = LWP_CreateThread(
        &l, hackThreadMain, hackParam, stack2, stackSize, prio);
#if THREAD_DEBUG
    exiPrintf("OSCreateThread(t=%08X f=%08X(%08X) s=%08X[%08X] p=%d a=%04X) => %08X\n",
        (u32)thread, (u32)func, (u32)param, (u32)stack2,
        (u32)stackSize, (u32)priority, (u32)attr, (u32)l);
#endif
    if(!r) addDolphinThread(thread, l);
    else exiPrintf(" *** ERROR *** LWP_CreateThread: %d\n", r);
    //LWP_YieldThread();
    switchToGame();
    return ok;
}

void OSCancelThread_hook(OSThread *thread) {
    /* Aborts the specified thread. If suspended on a thread
     * queue, thread is removed from that queue. All mutexes
     * held by thread will be released.  All threads waiting
     * to be joined (via OSJoinThread()) will be made runnable.
     * Any thread with the "detached" attribute set will be
     * removed from the run queue immediately.
     * If the "detached" attribute is not set, thread will be
     * removed from the run queue only when any joined threads
     * start running.
     */
    //LWP doesn't seem to have a public function for this...
    switchToOgc();
#if THREAD_DEBUG
    exiPrintf("OSCancelThread(%08X -> %08X)\n", (u32)thread,
        getDolphinThread(thread));
#endif
    LWP_SuspendThread(getDolphinThread(thread));
    LWP_SetThreadPriority(getDolphinThread(thread), 0);
    switchToGame();
}

void OSResumeThread_hook(OSThread *thread) {
    /* Resumes the specified thread (incremental).
     * The operating system keeps track of how many times
     * OSSuspendThread() has been called. A call to
     * OSResumeThread()will decrement that count. As long as
     * the count is greater than zero, the thread will not be
     * scheduled to run.
     */
    switchToOgc();
#if THREAD_DEBUG
    exiPrintf("OSResumeThread(%08X -> %08X)\n", (u32)thread,
        getDolphinThread(thread));
#endif
    LWP_ResumeThread(getDolphinThread(thread));
    switchToGame();
}

void OSSuspendThread_hook(OSThread *thread) {
    /* Suspends the specified thread. (incremental).
     * The operating system keeps track of how many times
     * OSSuspendThread() has been called. A call to
     * OSResumeThread() will decrement that count. While the
     * count is above zero, the thread will not be scheduled
     * to run.
     */
    switchToOgc();
#if THREAD_DEBUG
    exiPrintf("OSSuspendThread(%08X -> %08X)\n", (u32)thread,
        getDolphinThread(thread));
#endif
    LWP_SuspendThread(getDolphinThread(thread));
    switchToGame();
}

void OSSleepThread_hook(OSThreadQueue *queue) {
    /* Suspends the current thread and insert it into the
     * specified thread queue. It will remain in a
     * suspended state until OSWakeupThread() is called
     * on this queue.
     */
    switchToOgc();
#if THREAD_DEBUG
    exiPrintf("[%08X] OSSleepThread(%08X -> %08X)\n",
        (u32)LWP_GetSelf(), (u32)queue, LWP_GetSelf());
#endif
    //set_msr(get_msr() | 0x8000); //enable interrupts
    LWP_ThreadSleep(*getQueue(queue));
    //set_msr(get_msr() & ~0x8000); //disable interrupts
    switchToGame();
}

void OSWakeupThread_hook(OSThreadQueue *queue) {
    /* Wakes ALL the threads in the specified thread queue.
     * They will be scheduled according to their priority
     * and the order in which they entered the queue.
     */
    switchToOgc();
#if THREAD_DEBUG
    exiPrintf("[%08X] OSWakeupThread(%08X) msr=%08X INSTR=%08X INMTR=%08X\n",
        (u32)LWP_GetSelf(),
        (u32)queue, get_msr(), PI_INSTR, PI_INMTR);
#endif
    LWP_ThreadBroadcast(*getQueue(queue));
    LWP_YieldThread();
    switchToGame();
}

OSThread* OSGetCurrentThread_hook() {
    /* Gets pointer to the current thread. */
    switchToOgc();
    OSThread *r = getDolphinThreadForLwp(LWP_GetSelf());
    switchToGame();
    return r;
}

void OSInitThreadQueue_hook(OSThreadQueue *queue) {
    //no need to init, getQueue does that
    switchToOgc();
    getQueue(queue);
#if THREAD_DEBUG
    exiPrintf("OSInitThreadQueue(%08X) => %08X self=%08X\n",
        (u32)queue, (u32)getQueue(queue), LWP_GetSelf());
#endif

    //reproduce the original OSInitThreadQueue()
    queue->head = NULL;
    queue->tail = NULL;
    switchToGame();
}


void OSInitMessageQueue_hook(void *queue, u32 *msgArray, s32 msgCount) {
    //msgArray is memory to hold the messages.
    //libogc's method allocates its own instead.
    switchToOgc();
    mqbox_t *mq = addDolphinMsgQueue(queue, msgCount);
#if THREAD_DEBUG
    exiPrintf("OSInitMessageQueue(%08X, %d) => %08X\n",
        queue, msgCount, mq);
#endif
    switchToGame();
}

BOOL OSSendMessage_hook(void *queue, u32 msg, u32 flags) {
    switchToOgc();
    //dolphin flags are almost the same except inverted.
    //also, there's only one flag.
#if THREAD_DEBUG
    exiPrintf("OSSendMessage(%08X, %08X, %d)\n", queue, msg, flags);
#endif
    u32 mqFlags = (
        ((flags & 1) ? MQ_MSG_BLOCK : MQ_MSG_NOBLOCK)
    );
    mqbox_t* mq = NULL;
    while(!mq) {
        mq = getMsgQueue(queue);
        if(mq) break;
        LWP_YieldThread();
    }
    BOOL r = mq ? MQ_Send(*mq, (mqmsg_t)msg, mqFlags) : false;
    switchToGame();
    return r;
}

BOOL OSReceiveMessage_hook(void *queue, u32 *msg, u32 flags) {
    switchToOgc();
    //dolphin flags are almost the same except inverted.
    //also, there's only one flag.
#if THREAD_DEBUG
    exiPrintf("OSReceiveMessage(%08X, %08X, %d)\n", queue, msg, flags);
#endif
    u32 mqFlags = (
        ((flags & 1) ? MQ_MSG_BLOCK : MQ_MSG_NOBLOCK)
    );
    mqbox_t* mq = NULL;
    while(!mq) {
        mq = getMsgQueue(queue);
        if(mq) break;
        LWP_YieldThread();
    }
    BOOL r = mq ? MQ_Receive(*mq, (mqmsg_t*)msg, mqFlags) : false;
#if THREAD_DEBUG
    exiPrintf("OSReceiveMessage(%08X, %08X, %d) result=%d\n",
        queue, msg, flags, r);
#endif
    switchToGame();
    return r;
}

static uint (*gameSetInterruptMask)(uint, uint) =
    (uint(*)(uint, uint))0x8024386c;
#define prevIntrMask (*(u32*)0x800000c4)
#define curIntrMask  (*(u32*)0x800000c8)

u32 _gameMaskInterrupts(u32 flags) {
    //u32 irq = IRQ_Disable();
    __MaskIrq(flags);

    /*uint uVar1 = prevIntrMask;
    uint uVar2 = prevIntrMask | curIntrMask;
    prevIntrMask = flags | prevIntrMask;
    uint uVar3 = prevIntrMask | curIntrMask;
    for(uVar2 = flags & ~uVar2;
    uVar2 != 0;
    uVar2 = gameSetInterruptMask(uVar2,uVar3)) {
        //do nothing
    }*/

    //IRQ_Restore(irq);
#if THREAD_DEBUG
    exiPrintf("OSMaskInterrupts(%08X) => %08X\n", flags,
        get_msr());
#endif
    return prevIntrMask;
}

u32 _gameUnmaskInterrupts(u32 flags) {
    //u32 irq = IRQ_Disable();
    __UnmaskIrq(flags);

     //copy original logic
    /*uint prevMask = prevIntrMask;
    uint uVar1 = prevIntrMask | curIntrMask;
    prevIntrMask = prevIntrMask & ~flags;
    uint nowFlags = prevIntrMask | curIntrMask;
    for(uint commonFlags = flags & uVar1;
    commonFlags != 0;
    commonFlags = gameSetInterruptMask(commonFlags,nowFlags)) {
        //do nothing. this is ugly but it should work.
        //copied from ghidra which likes to make the loops
        //uglier than needed.
    }*/

    //IRQ_Restore(irq);
#if THREAD_DEBUG
    exiPrintf("OSUnmaskInterrupts(%08X) => %08X\n", flags,
        get_msr());
#endif
    return prevIntrMask;
}

u32 OSDisableInterrupts_hook() {
    switchToOgc();
    u32 r = IRQ_Disable();
    switchToGame();
    return r;
}
/*u32 OSEnableInterrupts_hook() {
    switchToOgc();
    u32 r = IRQ_Enable();
    switchToGame();
    return r;
}*/
void OSRestoreInterrupts_hook(u32 level) {
    switchToOgc();
    IRQ_Restore(level);
    switchToGame();
}

u32 OSMaskInterrupts_hook(u32 flags) {
    switchToOgc();
    int r = _gameMaskInterrupts(flags);
    switchToGame();
    return r;
}

u32 OSUnmaskInterrupts_hook(u32 flags) {
    switchToOgc();
    int r = _gameUnmaskInterrupts(flags);
    switchToGame();
    return r;
}

void OSSwitchThreadOrSomeShit_hook(int unused) {
    //param is always 0, return is ignored
    //this value is something to do with priority?
    while(*(vu32*)0x803dde88);
    LWP_YieldThread();
}
