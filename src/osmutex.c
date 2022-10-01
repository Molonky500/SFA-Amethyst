#include "main.h"

//these don't seem to be in the game.

//the game doesn't contain this
void OSYieldThread(void) {
    int irq = OSDisableInterrupts();
    SelectThread(true);
    OSRestoreInterrupts(irq);
}

void __OSPromoteThread(OSThread* thread, OSPriority priority) {
    do {
        //printf("%s(%08X, %d)\n", __FUNCTION__, (u32)thread, priority);
        if((thread->suspend > 0)
        || thread->priority <= priority) break;
        thread = _OSSetThreadPriority(thread, priority);
    } while (thread);
}

void OSInitMutex(OSMutex* mutex) {
    //OSReport("OSInitMutex(%08X)\n", mutex);
    _OSInitThreadQueue(&mutex->queue);
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
    u32 irq = OSDisableInterrupts();
    //OSReport("[%08X] OSLockMutex(%08X)\n", _OSGetCurrentThread(), mutex);
    OSThread *curThread = _OSGetCurrentThread();
    if(!curThread) {
        OSRestoreInterrupts(irq);
        return;
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
            _OSSleepThread(&mutex->queue);
            curThread->mutex = 0;
        }
    }
    OSRestoreInterrupts(irq);
}

void OSUnlockMutex(OSMutex* mutex) {
    //OSReport("[%08X] OSUnlockMutex(%08X)\n", _OSGetCurrentThread(), mutex);
    u32 irq = OSDisableInterrupts();
    OSThread *curThread = _OSGetCurrentThread();
    if(!curThread) {
        OSRestoreInterrupts(irq);
        return;
    }
    if(mutex->thread == curThread && --mutex->count == 0) {
        DequeueItem(&curThread->queueMutex, mutex);
        mutex->thread = NULL;
        if(curThread->priority < curThread->base) {
            curThread->priority = ___OSGetEffectivePriority(curThread);
        }
        _OSWakeupThread(&mutex->queue);
    }
    OSRestoreInterrupts(irq);
}

BOOL OSTryLockMutex(OSMutex* mutex) {
    bool locked = false;
    u32 irq   = OSDisableInterrupts();
    OSThread *curThread = _OSGetCurrentThread();
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
    _OSInitThreadQueue(&cond->queue);
}

void OSWaitCond(OSCond* cond, OSMutex* mutex) {
    s32 count;
    u32 irq = OSDisableInterrupts();
    OSThread *curThread = _OSGetCurrentThread();

    if(mutex->thread == curThread) {
        count = mutex->count;
        mutex->count = 0;

        DequeueItem(&curThread->queueMutex, mutex);
        mutex->thread = NULL;
        if(curThread->priority < curThread->base) {
            curThread->priority = ___OSGetEffectivePriority(curThread);
        }
        _OSDisableScheduler();
        _OSWakeupThread(&mutex->queue);
        _OSEnableScheduler();
        _OSSleepThread(&cond->queue);
        OSLockMutex(mutex);
        mutex->count = count;
    }
    OSRestoreInterrupts(irq);
}

void OSSignalCond(OSCond* cond) {
    _OSWakeupThread(&cond->queue);
}
