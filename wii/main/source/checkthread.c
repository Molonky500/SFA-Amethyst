#include "main.h"

#define CHECK_THREAD_STACK_SIZE 8192
#define CHECK_THREAD_PRIO 30 //31=lowest 0=highest
#define OS_THREAD_STACK_MAGIC 0xDEADBABE

OSThread checkThread;
static u8 checkThreadStack[CHECK_THREAD_STACK_SIZE];

void initCheckThread() {
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

void* checkThreadMain(void *param) {
    /** Background thread that scans for anomalies.
     */
    exiPrintf("checkThread online (%08X, stack %08X))n",
        (u32)&checkThread, (u32)checkThreadStack);
    while(1) {
        OSYieldThread();
        checkAlloc();
        checkGameHeaps();

        int iThread = 0;
        OSThread *thread = *(OSThread**)0x800000dc;
        while(thread) {
            u32 stackTop = (u32)thread->stackBase; //high addr
            u32 stackBot = (u32)thread->stackEnd;  //low  addr
            u32 sp       = (u32)thread->context.gpr[1];
            if(!(PTR_VALID(stackTop) && PTR_VALID(stackBot))) {
                exiPrintf(" *** ERROR *** thread %08X invalid stack (%08X-%08X)\n",
                    (u32)thread, stackBot, stackTop);
            }
            if(sp < stackBot || sp > stackTop) {
                exiPrintf(" *** ERROR *** stack overflow in thread %08X (%08X not in range %08X-%08X)\n",
                    (u32)thread, sp, stackBot, stackTop);
            }
            if(*(thread->stackEnd) != OS_THREAD_STACK_MAGIC) {
                exiPrintf(" *** ERROR *** stack corruption in thread %08X (magic=%08X)\n",
                    (u32)thread, *(thread->stackEnd));
            }
            thread = thread->linkActive.next;
            iThread++;
            if(iThread > 256) {
                exiPrintf(" *** ERROR *** infinite loop in thread link\n");
                break;
            }
        }
    }
}
