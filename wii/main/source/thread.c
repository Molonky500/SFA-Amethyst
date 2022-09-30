#include "main.h"

//the game doesn't contain this
void OSYieldThread(void) {
    int irq = OSDisableInterrupts();
    SelectThread(true);
    OSRestoreInterrupts(irq);
}

void __OSPromoteThread(OSThread* thread, OSPriority priority) {
    do {
        printf("%s(%08X, %d)\n", __FUNCTION__, (u32)thread, priority);
        if((thread->suspend > 0)
        || thread->priority <= priority) break;
        thread = SetEffectivePriority(thread, priority);
    } while (thread);
}
