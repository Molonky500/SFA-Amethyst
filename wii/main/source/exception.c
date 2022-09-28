#include "main.h"

static vu32 ogc_r2   = 0xDEAD106C;
static vu32 ogc_r13  = 0xDEAD106C;
static vu32 game_r2  = 0x803E6500;
static vu32 game_r13 = 0x803E31E0;

void switchToGame() {
    u32 irq = IRQ_Disable();
    if(get_r2() != game_r2) {
        ogc_r2 = get_r2();
        set_r2(game_r2);
        ogc_r13 = get_r13();
        set_r13(game_r13);
    }
    //set_msr(0x00009032);
    IRQ_Restore(irq);
}
void switchToOgc() {
    u32 irq = IRQ_Disable();
    //game ones never change, no need to save
    if(get_r2() == game_r2) {
        set_r2(ogc_r2);
        set_r13(ogc_r13);
    }
    //set_msr(0x0000B036);
    IRQ_Restore(irq);
}

void putHex(char *dst, u32 num) {
    //snprintf doesn't work in an exception handler
    //static const char *hex = "0123456789ABCDEF";
    static const char *hex = "0123456789abcdef";
    //lowercase is nice to paste into objdump | less
    for(int i=0; i<8; i++) {
        *(dst++) = hex[(num >> 28) & 0xF];
        num <<= 4;
    }
}


u32 (*__OSSetExceptionHandler)(u32 exception, void *handler) = 0x80240bc4;
void __exception_sethandler(u32 nExcept, void (*pHndl)(frame_context*));

void OSExceptionInit_hook() {
    //we repurpose the memory for some unused handlers
    //to store our trampolines and such, so we disable
    //the game's original method and install them ourselves.
    exiPrintf("%s\n", __FUNCTION__);

    *(u32*)0x803dddec = 0x80003000; //set exception handler table
    // *(u32*)0x803dde38 = 0x80003040; //set IRQ handler table

    static const void *excAddrs[] = {
        (void*)0xFFFFFFFF, //(void*)0x80000100, //reset (not connected)
        (void*)0xFFFFFFFF, //(void*)0x80000200, //machine check
        (void*)0x80000300, //DSI
        (void*)0x80000400, //ISI
        (void*)0x80000500, //External interrupt
        (void*)0x80000600, //alignment
        (void*)0x80000700, //program
        (void*)0x80000800, //FPU Unavailable
        (void*)0x80000900, //decrementer
        (void*)0x80000C00, //syscall
        (void*)0x80000D00, //trace
        (void*)0x80000F00, //perfmon
        (void*)0x80001300, //IABR (instruction breakpoint)
        (void*)0xFFFFFFFF, //(void*)0x80001400, //reserved
        (void*)0x80001700, //thermal
        (void*)0
    };
    for(int i=0; excAddrs[i]; i++) {
        if(excAddrs[i] == (void*)0xFFFFFFFF) continue;
        memcpy(excAddrs[i], 0x80240bf4, 0x98);
        //patch in exception code.
        //this is actually how the game does this.
        //in fact this is part of DolphinOS, so this is
        //probably how every game does this.
        u32 *patch = (u32*)excAddrs[i];
        patch[0x1A] = 0x38600000 | i;
        //patch out debugger hook
        patch[0x16] = 0x60000000;

        //80240c90 = OSDefaultExceptionHandler
        __OSSetExceptionHandler(i, (void*)0x80240c90);
    }

    DCInvalidateRange((void*)0x80000300, 0x80001800 - 0x80000300);
    ICInvalidateRange((void*)0x80000300, 0x80001800 - 0x80000300);

    exiPrintf("%s done\n", __FUNCTION__);
}

static const char *excNames[] = {
    "Reset", "MachineCheck", "DSI",
    "ISI", "External", "Alignment", "Program",
    "FPUnavail", "Decrementer", "Rsvd0A", "Rsvd0B",
    "Syscall", "Trace", "Rsvd0E", "Perfmon",
    "Rsvd10", "Rsvd11", "Rsvd12", "Breakpoint",
    "Rsvd14", "Rsvd15", "Rsvd16", "Thermal",
};

void gameExceptionHook(int exceptionCode, OSContext *ctx,
uint cause, void *addr) {
    static bool alreadyExc = false;
    while(alreadyExc);
    alreadyExc = true;

    char msg[1024];
    strcpy(msg, "ERR ........ ");
    putHex(&msg[4], exceptionCode);
    exiPuts(msg);
    exiPuts(excNames[exceptionCode & 0xF]);

    strcpy(msg, "\nCAUSE=........ ADDR=........ DAR=........\n");
    putHex(&msg[ 7], cause);
    putHex(&msg[21], addr);
    putHex(&msg[34], get_dar());
    exiPuts(msg);

    strcpy(msg, "XER=........ SRR0=........ SRR1=........ LR=........\n");
    putHex(&msg[ 4], ctx->xer);
    putHex(&msg[18], ctx->srr0);
    putHex(&msg[32], ctx->srr1);
    putHex(&msg[44], ctx->lr);
    exiPuts(msg);

    exiPuts("GPRs:\n");
    u32 *gpr = &ctx->gpr;
    for(int i=0; i<32; i += 4) {
        strcpy(msg, "........ ........ ........ ........\n");
        putHex(&msg[ 0], gpr[i]);
        putHex(&msg[ 9], gpr[i+1]);
        putHex(&msg[18], gpr[i+2]);
        putHex(&msg[27], gpr[i+3]);
        exiPuts(msg);
    }

    u32 *sp = (u32*)gpr[1];
    strcpy(msg, "-> ........\n");
    while((u32)sp >= 0x80000000 && (u32)sp <= 0x93FFFFFF) {
        putHex(&msg[3], sp[1]);
        exiPuts(msg);
        sp = (u32*)*sp;
    }

    //SYS_ResetSystem(SYS_SHUTDOWN,0,0);
    while(1);
}
