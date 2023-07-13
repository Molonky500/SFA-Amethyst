#include "main.h"

#define CHECK_THREAD_STACK_SIZE 8192
#define CHECK_THREAD_PRIO 30 //31=lowest 0=highest
#define OS_THREAD_STACK_MAGIC 0xDEADBABE
#define MAX_THREADS_CHECK 16

OSThread checkThread;
static u8 checkThreadStack[CHECK_THREAD_STACK_SIZE];

PrevThreadState prevThreadState[MAX_THREADS_CHECK];

PrevThreadState* findThread(OSThread *thread) {
    PrevThreadState *pState = NULL;
    PrevThreadState *empty = NULL;
    for(int i=0; i<MAX_THREADS_CHECK; i++) {
        if(prevThreadState[i].thread == thread) {
            pState = &prevThreadState[i];
            break;
        }
        else if(empty == NULL
        && prevThreadState[i].thread == NULL) {
            empty = &prevThreadState[i];
        }
    }
    //not found
    if(!pState) {
        if(!empty) PANIC("Too many threads!\n");
        exiPrintf("New thread %08X\n", thread);
        empty->thread   = thread;
        empty->stackTop = (u32)thread->stackBase;
        empty->stackBot = (u32)thread->stackEnd;
        empty->name     = NULL;
        pState = empty;
    }
    return pState;
}

void registerThreadForDebug(OSThread *thread, const char *name) {
    PrevThreadState *pState = findThread(thread);
    if(pState) pState->name = name;
}


void initCheckThread() {
    memset(prevThreadState, 0, sizeof(prevThreadState));
    OSCreateThread(&checkThread, checkThreadMain,
        NULL, checkThreadStack+CHECK_THREAD_STACK_SIZE,
        CHECK_THREAD_STACK_SIZE, CHECK_THREAD_PRIO, 0);
    OSResumeThread(&checkThread);
}

void checkGameHeaps() {
    //we can't check for a magic value because our alloc hook
    //runs late, so some allocations won't have it.
    for(int iHeap=0; iHeap<GAME_NUM_HEAPS; iHeap++) {
        GameHeap *heap = &gameHeaps[iHeap];
        int iEntry = 0;
        while(iEntry >= 0) {
            GameHeapEntry *entry = &heap->data[iEntry];
            if(!entry) break; //heaps not setup yet
            if(entry->type != HEAP_ENTRY_TYPE_FREE
            && entry->type != HEAP_ENTRY_TYPE_RAM) {
                exiPrintf(" *** ERROR *** heap corruption at %08X (type=%08X)\n",
                    (u32)entry, (u32)entry->type);
                PANIC("Heap corruption detected!\n");
            }
            iEntry = entry->next;
        }
    }
}

void checkThreads() {
    int iThread = 0;
    OSThread *thread = *(OSThread**)0x800000dc;
    while(thread) {
        u32 stackTop = (u32)thread->stackBase; //high addr
        u32 stackBot = (u32)thread->stackEnd;  //low  addr
        u32 sp       = (u32)thread->context.gpr[1];
        u32 pc       = (u32)thread->context.srr0;

        const char *name = "unknown";
        PrevThreadState *pState = findThread(thread);
        if(pState) {
            if(pState->name != NULL) name = pState->name;
            if(stackTop != pState->stackTop
            || stackBot != pState->stackBot) {
                if(pState->stackTop && pState->stackBot) {
                    //if prev were 0, the thread was just
                    //not set up yet.
                    exiPrintf(" *** ERROR *** thread %08X (%s) stack changed from %08X-%08X to %08X-%08X\n",
                        (u32)thread, name,
                        pState->stackBot, pState->stackTop,
                        stackBot, stackTop);
                }
                //just in case this is really a new thread reusing the
                //same OSThread struct
                pState->stackBot = stackBot;
                pState->stackTop = stackTop;
            }
        }

        if(!(PTR_VALID(stackTop) && PTR_VALID(stackBot))) {
            exiPrintf(" *** ERROR *** thread %08X (%s) invalid stack (%08X-%08X) PC=%08X\n",
                (u32)thread, name, stackBot, stackTop, pc);
            PANIC("Stack corruption detected!\n");
        }
        if(sp < stackBot || sp > stackTop) {
            exiPrintf(" *** ERROR *** stack overflow in thread %08X (%s) (%08X not in range %08X-%08X) PC=%08X\n",
                (u32)thread, name, sp, stackBot, stackTop, pc);
            PANIC("Stack overflow detected!\n");
        }
        if(*(thread->stackEnd) != OS_THREAD_STACK_MAGIC) {
            exiPrintf(" *** ERROR *** stack corruption in thread %08X (%s) (magic=%08X) PC=%08X\n",
                (u32)thread, name, *(thread->stackEnd), pc);
            PANIC("Stack corruption detected!\n");
        }
        thread = thread->linkActive.next;
        iThread++;
        if(iThread > 256) {
            exiPrintf(" *** ERROR *** infinite loop in thread link\n");
            PANIC("Thread corruption detected!\n");
            break;
        }
    }
}

void checkObjects() {
    void **pObjs = *(void***)0x803dcb88;
    int nObjs = *(int*)0x803dcb84;
    if(nObjs > 1000) {
        exiPrintf(" *** ERROR *** too many objects (%d)\n", nObjs);
        nObjs = 1000;
    }
    for(int i=0; i<nObjs; i++) {
        void *obj = pObjs[i];
        if(!obj) continue;
        if(!PTR_VALID(obj)) {
            exiPrintf(" *** ERROR *** invalid objptr %p\n", obj);
            continue;
        }
        float *pos = (float*)(obj + 0xC);
        if((isnan(pos[0]) || isnan(pos[1]) || isnan(pos[2]))
        || (isinf(pos[0]) || isinf(pos[1]) || isinf(pos[2]))) {
            exiPrintf(" *** ERROR *** Obj %p has invalid position\n", obj);
        }
    }
}

void checkIntegrity() {
    checkAlloc();
    checkGameHeaps();
    checkThreads();
    checkObjects();
}

void* checkThreadMain(void *param) {
    /** Background thread that scans for anomalies.
     */
    exiPrintf("checkThread online (%08X, stack %08X)\n",
        (u32)&checkThread, (u32)checkThreadStack);
    registerThreadForDebug(OSGetCurrentThread(), "check");
    while(!gIsSystemShuttingDown) {
        OSYieldThread();
        checkIntegrity();
    }
    exiPuts("checkThread shutting down\n");
}
