#include "main.h"

#define CHECK_THREAD_STACK_SIZE 8192
#define CHECK_THREAD_PRIO 30 //31=lowest 0=highest
#define OS_THREAD_STACK_MAGIC 0xDEADBABE
#define MAX_THREADS 16

OSThread checkThread;
static u8 checkThreadStack[CHECK_THREAD_STACK_SIZE];

typedef struct {
    OSThread *thread;
    u32 stackTop;
    u32 stackBot;
    const char *name;
} PrevThreadState;

PrevThreadState prevThreadState[MAX_THREADS];

static PrevThreadState* findThread(OSThread *thread) {
    PrevThreadState *pState = NULL;
    PrevThreadState *empty = NULL;
    for(int i=0; i<MAX_THREADS; i++) {
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
        if(!empty) {
            exiPuts(" *** ERROR *** too many threads!\n");
            return NULL;
        }
        else {
            exiPrintf("New thread %08X\n", thread);
            empty->thread   = thread;
            empty->stackTop = (u32)thread->stackBase;
            empty->stackBot = (u32)thread->stackEnd;
            empty->name     = NULL;
            pState = empty;
        }
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
            *(int*)0 = 0; //trigger a crash
        }
        if(sp < stackBot || sp > stackTop) {
            exiPrintf(" *** ERROR *** stack overflow in thread %08X (%s) (%08X not in range %08X-%08X) PC=%08X\n",
                (u32)thread, name, sp, stackBot, stackTop, pc);
            *(int*)0 = 0; //trigger a crash
        }
        if(*(thread->stackEnd) != OS_THREAD_STACK_MAGIC) {
            exiPrintf(" *** ERROR *** stack corruption in thread %08X (%s) (magic=%08X) PC=%08X\n",
                (u32)thread, name, *(thread->stackEnd), pc);
            *(int*)0 = 0; //trigger a crash
        }
        thread = thread->linkActive.next;
        iThread++;
        if(iThread > 256) {
            exiPrintf(" *** ERROR *** infinite loop in thread link\n");
            *(int*)0 = 0; //trigger a crash
            break;
        }
    }
}

void checkIntegrity() {
    checkAlloc();
    checkGameHeaps();
    checkThreads();
}

void* checkThreadMain(void *param) {
    /** Background thread that scans for anomalies.
     */
    exiPrintf("checkThread online (%08X, stack %08X)\n",
        (u32)&checkThread, (u32)checkThreadStack);
    registerThreadForDebug(OSGetCurrentThread(), "check");
    while(1) {
        OSYieldThread();
        checkIntegrity();
    }
}
