#include "main.h"

//these don't seem to be in the game.

void OSInitMutex(OSMutex* mutex) {
    OSInitThreadQueue(&mutex->queue);
    mutex->thread = 0;
    mutex->count = 0;
}

static void EnqueueTail(OSMutexQueue *queue, OSMutex *mutex) {
    OSMutex *prev = queue->tail;
    if (!prev) queue->head = mutex;
    else prev->link.next = mutex;
    mutex->link.prev = prev;
    mutex->link.next = NULL;
    queue->tail = mutex;
}

static void DequeueItem(OSMutexQueue *queue, OSMutex *mutex) {
    OSMutex *prev = mutex->link.prev;
    OSMutex *next = mutex->link.next;

    if(!next) queue->tail = prev;
    else next->link.prev  = prev;
    if(!prev) queue->head = next;
    else prev->link.next  = next;
}

void OSLockMutex(OSMutex* mutex) {
    if(!mutex) {
        PANIC("Invalid mutex!");
    }
    u32 irq = OSDisableInterrupts();
    OSThread *curThread = OSGetCurrentThread();
    if(!curThread) {
        PANIC("No current thread!");
    }

    while(1) {
        OSThread *ownerThread = ((volatile OSMutex*)mutex)->thread;
        if(!ownerThread) {
            mutex->thread = curThread;
            mutex->count++;
            EnqueueTail(&curThread->queueMutex, mutex);
            break;
        }
        else if(ownerThread == curThread) {
            mutex->count++;
            break;
        }
        else {
            curThread->mutex = mutex;
            __OSPromoteThread(mutex->thread, curThread->priority);
            OSSleepThread(&mutex->queue);
            curThread->mutex = 0;
        }
    }
    OSRestoreInterrupts(irq);
}

void OSUnlockMutex(OSMutex* mutex) {
    u32 irq = OSDisableInterrupts();
    OSThread *curThread = OSGetCurrentThread();
    if(mutex->thread == curThread && --mutex->count == 0) {
        DequeueItem(&curThread->queueMutex, mutex);
        mutex->thread = NULL;
        if(curThread->priority < curThread->base) {
            curThread->priority = __OSGetEffectivePriority(curThread);
        }
        OSWakeupThread(&mutex->queue);
    }
    OSRestoreInterrupts(irq);
}

bool OSTryLockMutex(OSMutex* mutex) {
    bool locked = false;
    u32 irq   = OSDisableInterrupts();
    OSThread *curThread = OSGetCurrentThread();
    if(!mutex->thread) {
        mutex->thread = curThread;
        mutex->count++;
        EnqueueTail(&curThread->queueMutex, mutex);
        locked = true;
    }
    else if(mutex->thread == curThread) {
        mutex->count++;
        locked = true;
    }
    OSRestoreInterrupts(irq);
    return locked;
}

void OSInitCond(OSCond* cond) {
    OSInitThreadQueue(&cond->queue);
}

void OSWaitCond(OSCond* cond, OSMutex* mutex) {
    s32 count;
    u32 irq = OSDisableInterrupts();
    OSThread *curThread = OSGetCurrentThread();

    if(mutex->thread == curThread) {
        count = mutex->count;
        mutex->count = 0;

        DequeueItem(&curThread->queueMutex, mutex);
        mutex->thread = NULL;
        if(curThread->priority < curThread->base) {
            curThread->priority = __OSGetEffectivePriority(curThread);
        }
        OSDisableScheduler();
        OSWakeupThread(&mutex->queue);
        OSEnableScheduler();
        OSSleepThread(&cond->queue);
        OSLockMutex(mutex);
        mutex->count = count;
    }
    OSRestoreInterrupts(irq);
}

void OSSignalCond(OSCond* cond) {
    OSWakeupThread(&cond->queue);
}
