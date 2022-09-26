#include "main.h"

u32 (*__OSSetExceptionHandler)(u32 exception, void *handler) = 0x80240bc4;

void OSExceptionInit_hook() {
    //we repurpose the memory for some unused handlers
    //to store our trampolines and such, so we disable
    //the game's original method and install them ourselves.
    exiPrintf("%s\n", __FUNCTION__);

    static const void *excAddrs[] = {
        (void*)0xFFFFFFFF, //(void*)0x80000100, //reset (not connected)
        (void*)0xFFFFFFFF, //(void*)0x80000200, //machine check
        (void*)0x80000300, //DSI
        (void*)0x80000400, //ISI
        (void*)0x80000500, //External interrupt
        (void*)0xFFFFFFFF, //(void*)0x80000600, //alignment
        (void*)0xFFFFFFFF, //(void*)0x80000700, //program
        (void*)0x80000800, //FPU Unavailable
        (void*)0x80000900, //decrementer
        (void*)0x80000C00, //syscall
        (void*)0xFFFFFFFF, //(void*)0x80000D00, //trace
        (void*)0x80000F00, //perfmon
        (void*)0x80001300, //IABR
        (void*)0xFFFFFFFF, //(void*)0x80001400, //reserved
        (void*)0x80001700, //thermal
        (void*)0
    };
    for(int i=0; excAddrs[i]; i++) {
        if(excAddrs[i] == (void*)0xFFFFFFFF) continue;
        memcpy(excAddrs[i], 0x80240bf4, 0x98);
        //patch in exception code.
        //this is actually how the game does this.
        u32 *patch = (u32*)excAddrs[i];
        patch[0x1A] = 0x38600000 | i;
        //patch out debugger hook
        patch[0x16] = 0x60000000;
    }

    for(int exception = 0; exception < 0xf; exception = exception + 1) {
        //80240c90 = OSDefaultExceptionHandler
        __OSSetExceptionHandler(exception,(void*)0x80240c90);
    }

    DCInvalidateRange((void*)0x80000300, 0x80001800 - 0x80000300);
    ICInvalidateRange((void*)0x80000300, 0x80001800 - 0x80000300);

    exiPrintf("%s done\n", __FUNCTION__);
}
