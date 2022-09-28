#include "main.h"

//pointers to Dolphin OS functions
BOOL (*OSCreateThread)(
    OSThread*  thread,
    void*    (*func)(void*),
    void*      param,
    void*      stackBase,
    u32        stackSize,
    OSPriority priority,
    u16        attribute) = 0x802462a8;
s32 (*OSResumeThread)(OSThread* thread) = 0x80246668;
s32 (*OSSuspendThread)(OSThread* thread) = 0x802468f0;
OSThread* (*OSGetCurrentThread)(void) = 0x80245d88;
BOOL (*OSSetThreadPriority)(OSThread* thread, OSPriority priority) = 0x80245eb8;
void (*SelectThread)(BOOL) = 0x80246078;
void (*__OSReschedule)(void) = 0x80246278;
void (*OSSleepThread)(OSThreadQueue* queue) = 0x80246a60;
void (*OSWakeupThread)(OSThreadQueue* queue) = 0x80246b4c;
void (*OSInitThreadQueue)(OSThreadQueue* queue) = 0x80245d78;
int (*OSDisableInterrupts)(void) = 0x8024377c;
int (*OSEnableInterrupts)(void) = 0x80243790;
int (*OSRestoreInterrupts)(int) = 0x802437a4;

mapDolToLwpThread_t mapDolToLwpThread[MAX_THREADS];


//the game doesn't contain this
void OSYieldThread(void) {
    int irq = OSDisableInterrupts();
    SelectThread(true);
    OSRestoreInterrupts(irq);
}

void initThreads() {
    memset(mapDolToLwpThread, 0, sizeof(mapDolToLwpThread));

    LWP_CreateThread_hook = LWP_CreateThread_dol;
    LWP_SuspendThread_hook = LWP_SuspendThread_dol;
    LWP_ResumeThread_hook = LWP_ResumeThread_dol;
    LWP_ThreadIsSuspended_hook = LWP_ThreadIsSuspended_dol;
    LWP_GetSelf_hook = LWP_GetSelf_dol;
    LWP_SetThreadPriority_hook = LWP_SetThreadPriority_dol;
    LWP_YieldThread_hook = LWP_YieldThread_dol;
    LWP_Reschedule_hook = LWP_Reschedule_dol;
    LWP_JoinThread_hook = LWP_JoinThread_dol;
    LWP_InitQueue_hook = LWP_InitQueue_dol;
    LWP_CloseQueue_hook = LWP_CloseQueue_dol;
    LWP_ThreadSleep_hook = LWP_ThreadSleep_dol;
    LWP_ThreadSignal_hook = LWP_ThreadSignal_dol;
    LWP_ThreadBroadcast_hook = LWP_ThreadBroadcast_dol;
    MQ_Init_hook = MQ_Init_dol;
    MQ_Close_hook = MQ_Close_dol;
    MQ_Send_hook = MQ_Send_dol;
    MQ_Jam_hook = MQ_Jam_dol;
    MQ_Receive_hook = MQ_Receive_dol;

    exiPrintf("initThreads OK\n");
}

mapDolToLwpThread_t* getFreeLwpThreadSlot() {
    for(int i=0; i<MAX_THREADS; i++) {
        if(mapDolToLwpThread[i].dThread == NULL) {
            return &mapDolToLwpThread[i];
        }
    }
    exiPrintf(" *** ERROR *** No free lwpThread slot\n");
    return NULL;
}

s32 LWP_CreateThread_dol(lwp_t *thethread,
void* (*entry)(void *),void *arg,void *stackbase,
u32 stack_size,u8 prio) {
    //we use this list to keep track of whether we need
    //to free the stack when the thread exits.
    //XXX actually do that.
    mapDolToLwpThread_t *tMap = getFreeLwpThreadSlot();
    if(!tMap) return -1;

    //alloc an OSThread
    OSThread *dThread = malloc(sizeof(OSThread));
    tMap->dThread = dThread;
    if(!dThread) {
        exiPrintf(" *** ERROR *** malloc failed for %s\n",
            __FUNCTION__);
        return -1;
    }

    //alloc a stack if needed
    if(!stack_size) stack_size = 8192; //default size
    if(stackbase) tMap->allocatedStack = false;
    else {
        tMap->allocatedStack = true;
        stackbase = malloc(stack_size);
        if(!stackbase) {
            exiPrintf(" *** ERROR *** malloc failed for %s stack\n",
                __FUNCTION__);
            free(dThread);
            tMap->dThread = NULL;
            return -1;
        }
    }

    //map LWP priority to Dolphin priority
    prio = 31 - (prio >> 2);

    //create the DolphinOS thread
    switchToGame();
    int ok = OSCreateThread(dThread, entry, arg,
        stackbase, stack_size, prio, 0);
    switchToOgc();
    if(!ok) {
        exiPrintf(" *** ERROR *** OSCreateThread failed for %s\n",
            __FUNCTION__);
        free(dThread);
        tMap->dThread = NULL;
        return -1;
    }

    #if THREAD_DEBUG
        exiPrintf("LWP hook: created OSThread %08X\n", dThread);
    #endif

    *(OSThread**)thethread = dThread;

    //OSCreateThread creates the thread suspended.
    //LWP_CreateThread does not.
    switchToGame();
    OSResumeThread(dThread);
    switchToOgc();

    return 0;
}

s32 LWP_SuspendThread_dol(lwp_t thethread) {
    if(!PTR_VALID(thethread)) {
        exiPrintf(" *** ERROR *** Invalid thread %08X in %s\n",
            thethread, __FUNCTION__);
        return -1;
    }
    OSThread *dThread = (OSThread*)thethread;
    switchToGame();
    OSSuspendThread(dThread);
    switchToOgc();
    return 0;
}

s32 LWP_ResumeThread_dol(lwp_t thethread) {
    if(!PTR_VALID(thethread)) {
        exiPrintf(" *** ERROR *** Invalid thread %08X in %s\n",
            thethread, __FUNCTION__);
        return -1;
    }
    OSThread *dThread = (OSThread*)thethread;
    switchToGame();
    OSResumeThread(dThread);
    switchToOgc();
    return 0;
}

BOOL LWP_ThreadIsSuspended_dol(lwp_t thethread) {
    if(!PTR_VALID(thethread)) {
        exiPrintf(" *** ERROR *** Invalid thread %08X in %s\n",
            thethread, __FUNCTION__);
        return -1;
    }
    OSThread *dThread = (OSThread*)thethread;
    return dThread->suspend > 0;
}

lwp_t LWP_GetSelf_dol(void) {
    switchToGame();
    lwp_t r = (lwp_t)OSGetCurrentThread();
    switchToOgc();
    return r;
}

void LWP_SetThreadPriority_dol(lwp_t thethread,u32 prio) {
    if(!PTR_VALID(thethread)) {
        exiPrintf(" *** ERROR *** Invalid thread %08X in %s\n",
            thethread, __FUNCTION__);
        return;
    }
    OSThread *dThread = (OSThread*)thethread;

    //map LWP priority to Dolphin priority
    prio = 31 - (prio >> 2);
    switchToGame();
    OSSetThreadPriority(dThread, prio);
    switchToOgc();
}

void LWP_YieldThread_dol(void) {
    switchToGame();
    OSYieldThread();
    switchToOgc();
}

void LWP_Reschedule_dol(u32 prio) {
    switchToGame();
    __OSReschedule();
    switchToOgc();
}

s32 LWP_JoinThread_dol(lwp_t thethread,void **value_ptr) {
    if(!PTR_VALID(thethread)) {
        exiPrintf(" *** ERROR *** Invalid thread %08X in %s\n",
            thethread, __FUNCTION__);
        return -1;
    }
    OSThread *dThread = (OSThread*)thethread;

    //OSJoinThread isn't in this game
    while(dThread->state != OS_THREAD_STATE_MORIBUND) {
        switchToGame();
        OSYieldThread();
        switchToOgc();
    }
    if(value_ptr) *value_ptr = dThread->val;
    //XXX do we need to free dThread here?

    return 0;
}

s32 LWP_InitQueue_dol(lwpq_t *thequeue) {
    OSThreadQueue *dolQ = malloc(sizeof(OSThreadQueue));
    if(!dolQ) {
        exiPrintf(" *** ERROR *** malloc failed for %s\n",
            __FUNCTION__);
        return -1;
    }
    //this is all OSInitThreadQueue does
    dolQ->head = NULL;
    dolQ->tail = NULL;

    exiPrintf("%s: %08X -> %08X\n", __FUNCTION__, thequeue, dolQ);
    *(OSThreadQueue**)thequeue = dolQ;
    return 0;
}

void LWP_CloseQueue_dol(lwpq_t thequeue) {
    if(!PTR_VALID(thequeue)) {
        exiPrintf(" *** ERROR *** Invalid queue %08X in %s\n",
            thequeue, __FUNCTION__);
        return;
    }
    exiPrintf("%s: %08X\n", __FUNCTION__, thequeue);
    free((OSThreadQueue*)thequeue);
}

s32 LWP_ThreadSleep_dol(lwpq_t thequeue) {
    if(!PTR_VALID(thequeue)) {
        exiPrintf(" *** ERROR *** Invalid queue %08X in %s\n",
            thequeue, __FUNCTION__);
        return -1;
    }
    OSThreadQueue *dolQ = (OSThreadQueue*)thequeue;
    exiPrintf("OSSleepThread(%08X) self=%08X\n", dolQ,
        OSGetCurrentThread());
    /*if(!(dolQ->head || dolQ->tail)) {
        //in theory this is valid, just waiting for another
        //thread to wake us up...
        exiPrintf("Sleep thread on EMPTY queue %08X\n", dolQ);
        OSYieldThread();
        return 0;
    }*/
    switchToGame();
    OSSleepThread(dolQ);
    switchToOgc();
    exiPrintf("DONE OSSleepThread(%08X) self=%08X\n", dolQ,
        OSGetCurrentThread());
    return 0;
}

void LWP_ThreadSignal_dol(lwpq_t thequeue) {
    if(!PTR_VALID(thequeue)) {
        exiPrintf(" *** ERROR *** Invalid queue %08X in %s\n",
            thequeue, __FUNCTION__);
        return;
    }

    //XXX we only have OSWakeupThread which wakes ALL threads
    //on the queue, when this function is only supposed
    //to wake ONE thread...
    OSThreadQueue *dolQ = (OSThreadQueue*)thequeue;
    switchToGame();
    OSWakeupThread(dolQ);
    switchToOgc();
}

void LWP_ThreadBroadcast_dol(lwpq_t thequeue) {
    if(!PTR_VALID(thequeue)) {
        exiPrintf(" *** ERROR *** Invalid queue %08X in %s\n",
            thequeue, __FUNCTION__);
        return;
    }

    OSThreadQueue *dolQ = (OSThreadQueue*)thequeue;
    switchToGame();
    OSWakeupThread(dolQ);
    switchToOgc();
}
