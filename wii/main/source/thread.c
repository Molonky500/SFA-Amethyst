#include "main.h"

//the game doesn't contain this
void OSYieldThread(void) {
    int irq = OSDisableInterrupts();
    SelectThread(true);
    OSRestoreInterrupts(irq);
}
