#include "main.h"

FILE *crashLog=NULL, *crashDump=NULL;

void OSExceptionInit_hook() {
    //we repurpose the memory for some unused handlers
    //to store our trampolines and such, so we disable
    //the game's original method and install them ourselves.
    //exiPrintf("%s\n", __FUNCTION__);

    SET_SCREEN_SOLID_YUV(105, 212, 134); //magenta
    *(u32*)0x803dddec = 0x80003000; //set exception handler table
    // *(u32*)0x803dde38 = 0x80003040; //set IRQ handler table

    static void *excAddrs[] = {
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
        //copy exception handler.
        memcpy(excAddrs[i], (void*)0x80240bf4, 0x98);
        //patch to put exception ID in r3.
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

    //exiPrintf("%s done\n", __FUNCTION__);
}

void gameExceptionInit() {
    //opening these after a crash happens is unreliable,
    //so open them now.
    char path[1024];
    snprintf(path, sizeof(path), "%s/crash.log", gameRootDir);
    crashLog  = fopen(path, "wt");
    if(crashLog) exiPuts("Opened crashlog file OK\n");
    else exiPrintf("Open crashlog FAIL: %d\n", errno);

    snprintf(path, sizeof(path), "%s/crash.dmp", gameRootDir);
    crashDump = fopen(path, "wb");
    if(crashDump) exiPuts("Opened crashdump file OK\n");
    else exiPrintf("Open crashdump FAIL: %d\n", errno);
}

static const char *excNames[] = {
    "Reset", "MachineCheck", "DSI",
    "ISI", "External", "Alignment", "Program",
    "FPUnavail", "Decrementer", "Rsvd0A", "Rsvd0B",
    "Syscall", "Trace", "Rsvd0E", "Perfmon",
    "Rsvd10", "Rsvd11", "Rsvd12", "Breakpoint",
    "Rsvd14", "Rsvd15", "Rsvd16", "Thermal",
};

void flashDiscLedForever() {
    while(1) {
        SET_DISC_LED(1);
        udelay(100000000);
        SET_DISC_LED(0);
        udelay(100000000);
    }
}

void writeCrashLog(int exceptionCode, OSContext *ctx,
uint cause, void *addr, u32 msr) {
    //OSEnableScheduler();
    //OSEnableInterrupts();
    if(crashDump) {
        exiPuts("Writing crash dump\n");
        fwrite((void*)0x80000000, 1024, 24*1024, crashDump);
        fwrite((void*)0x90000000, 1024, 64*1024, crashDump);
        exiPuts("Wrote crash dump\n");
        fclose(crashDump);
    }

    if(!crashLog) return;
    exiPuts("Writing crash log\n");
    //I don't know why, but this fputs() makes the following
    //fprintf()s work
    fputs("Star Fox Adventures crash log\n", crashLog);
    fprintf(crashLog, "Error %d (%s)\n", exceptionCode,
        excNames[exceptionCode & 0xF]);
    fprintf(crashLog, "CAUSE=%08X ADDR=%08X DAR=%08X MSR=%08X\n",
        cause, (u32)addr, get_dar(), msr);
    fprintf(crashLog, "XER=%08X SRR0=%08X SRR1=%08X LR=%08X\n",
        ctx->xer, ctx->srr0, ctx->srr1, ctx->lr);
    fprintf(crashLog, "IRQ=%d #%d Cause %08X %08X\n",
        irqHandlerDepth, curIrqHandler, lastIrqCause,
        lastIrqCause2);

    u32 *gpr = (u32*)&ctx->gpr;
    for(int i=0; i<32; i += 4) {
        fprintf(crashLog, "GPR %2d:%08X %2d:%08X %2d:%08X %2d:%08X\n",
            i, gpr[i], i+1, gpr[i+1], i+2, gpr[i+2], i+3, gpr[i+3]);
    }

    u32 *sp = (u32*)gpr[1];
    while((u32)sp >= 0x80000000 && (u32)sp <= 0x93FFFFFF) {
        fprintf(crashLog, "Trace: %08X\n", sp[1]);
        sp = (u32*)*sp;
    }
    exiPuts("Wrote crash log\n");
    fclose(crashLog);
}

//void gameExceptionHook(int exceptionCode, OSContext *ctx,
//uint cause, void *addr) {
void gameExceptionHook() {
    static bool alreadyExc = false;
    if(alreadyExc) flashDiscLedForever();
    alreadyExc = true;

    int exceptionCode = *(s16*)0x803dda40;
    OSContext *ctx = *(OSContext**)0x803dda3c;
    uint cause = *(uint*)0x803dda38;
    void *addr = *(void**)0x803dda34;

    u32 msr = mfmsr();

    char msg[1024];
    strcpy(msg, "ERR ........ ");
    putHex(&msg[4], exceptionCode);
    exiPuts(msg);
    exiPuts(excNames[exceptionCode & 0xF]);

    strcpy(msg, "\nCAUSE=........ ADDR=........ DAR=........ MSR=........\n");
    putHex(&msg[ 7], cause);
    putHex(&msg[21], (u32)addr);
    putHex(&msg[34], get_dar());
    putHex(&msg[47], msr);
    exiPuts(msg);

    if(PTR_VALID(ctx)) {
        strcpy(msg, "XER=........ SRR0=........ SRR1=........ LR=........\n");
        putHex(&msg[ 4], ctx->xer);
        putHex(&msg[18], ctx->srr0);
        putHex(&msg[32], ctx->srr1);
        putHex(&msg[44], ctx->lr);
        exiPuts(msg);
    }
    else exiPuts("NO CTX\n");

    strcpy(msg, "IRQ=........ #........ Cause ........ ........\n");
    putHex(&msg[ 4], irqHandlerDepth);
    putHex(&msg[14], curIrqHandler);
    putHex(&msg[29], lastIrqCause);
    putHex(&msg[38], lastIrqCause2);
    exiPuts(msg);

    if(PTR_VALID(ctx)) {
        exiPuts("GPRs:\n");
        u32 *gpr = (u32*)&ctx->gpr;
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
    }

    writeCrashLog(exceptionCode, ctx, cause, addr, msr);
    dumpGameHeaps();
    checkIntegrity();
    SET_SCREEN_SOLID_YUV(76, 84, 255); //red
    exiPuts("Crash log written OK.\n");
    flashDiscLedForever();

    //SYS_ResetSystem(SYS_SHUTDOWN,0,0);
    while(1);
}
