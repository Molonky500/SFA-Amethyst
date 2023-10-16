#include "main.h"

FILE *crashLog=NULL, *crashDump=NULL;

void OSExceptionInit_hook() {
    //we repurpose the memory for some unused handlers
    //to store our trampolines and such, so we disable
    //the game's original method and install them ourselves.
    iguanaPutsNoFlush("OSExceptionInit_hook\r\n");
    SET_DEBUG_PORT(__LINE__);

    SET_SCREEN_SOLID_YUV(105, 212, 134); //magenta
    //udelay(500000);
    *(u32*)0x803dddec = 0x80003000; //set exception handler table
    // *(u32*)0x803dde38 = 0x80003040; //set IRQ handler table
    SET_DEBUG_PORT(__LINE__);

    static void *excAddrs[] = {
        (void*)0xFFFFFFFF, //(void*)0x80000100, //reset (not connected)
        (void*)0xFFFFFFFF, //(void*)0x80000200, //machine check
        (void*)0x80000300, //DSI
        (void*)0x80000400, //ISI
        (void*)0x80000500, //External interrupt
        (void*)0xFFFFFFFF, //(void*)0x80000600, //alignment
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
    iguanaPutsNoFlush("About to install exception handlers\r\n");
    for(int i=0; excAddrs[i]; i++) {
        exiPrintf("Handler[%2d] = 0x%08X\r\n", i, excAddrs[i]);
    }

    for(int i=0; excAddrs[i]; i++) {
        if(excAddrs[i] == (void*)0xFFFFFFFF) {
            __OSSetExceptionHandler(i, (void*)interactiveDebugger);
            continue;
        }
        //copy exception handler.
        //memcpy(excAddrs[i], (void*)0x80240bf4, 0x98);
        u32 *dst = (u32*)excAddrs[i];
        u32 *src = (u32*)0x80240bf4;
        for(int j=0; j<0x98 >> 2; j++) dst[j] = src[j];
        //patch to put exception ID in r3.
        //this is actually how the game does this.
        //in fact this is part of DolphinOS, so this is
        //probably how every game does this.
        u32 *patch = (u32*)excAddrs[i];
        patch[0x1A] = 0x38600000 | i;
        //patch out debugger hook
        patch[0x16] = 0x60000000;

        //80240c90 = OSDefaultExceptionHandler
        SET_DEBUG_PORT(__LINE__);
        __OSSetExceptionHandler(i, (void*)0x80240c90);
        SET_DEBUG_PORT(__LINE__);
    }

    //This is called immediately after this function, but,
    //once exception handlers are installed, this needs to
    //be called before we ever try to flush cache (which
    //exiPuts does) or it will crash.
    //There's no harm calling it twice, so let's just do it.
    iguanaPutsNoFlush("Init syscall...\r\n");
    void (*__OSInitSystemCall)(void) = 0x80245bec;
    __OSInitSystemCall();
    *(u32*)0x80000C10 = 0x60000000; //no idea but this sync freezes

    iguanaPutsNoFlush("Flush...\r\n");
    DCFlushRangeNoSync(0x80000000, 0x1800);
    _sync();
    __asm__ __volatile__ ("isync");
    ICInvalidateRange(0x80000000, 0x1800);

    SET_DEBUG_PORT(__LINE__);
    iguanaPutsNoFlush("OSExceptionInit_hook done\r\n");
    SET_DEBUG_PORT(__LINE__);

    exiPrintf("INTSR=%08X INTMR=%08X DAR=%08X DSISR=%08X\r\n",
        *(vu32*)0xCC003000, *(vu32*)0xCC003004,
        mfspr(19), mfspr(18));
    for(u32 addr = 0x80003000; addr < 0x800030C0; addr += 16) {
        u32 *d = (u32*)addr;
        exiPrintf("%08X: %08X %08X %08X %08X\r\n", addr,
            d[0], d[1], d[2], d[3]);
    }
}

void gameExceptionInit() {
    //opening these after a crash happens is unreliable,
    //so open them now.
    char path[1024];
    snprintf(path, sizeof(path), "%s/crash.log", gameRootDir);
    crashLog  = fopen(path, "wt");
    if(crashLog) exiPuts("Opened crashlog file OK\r\n");
    else exiPrintf("Open crashlog FAIL: %d\r\n", errno);

    snprintf(path, sizeof(path), "%s/crash.dmp", gameRootDir);
    crashDump = fopen(path, "wb");
    if(crashDump) exiPuts("Opened crashdump file OK\r\n");
    else exiPrintf("Open crashdump FAIL: %d\r\n", errno);
}

static const char *excNames[] = {
    /* none, */   "Reset",       "MachineCheck", "DSI",
    "ISI",        "External",    "Alignment",    "Program",
    "FPUnavail",  "Decrementer", "Rsvd0A",       "Rsvd0B",
    "Syscall",    "Trace",       "Rsvd0E",       "Perfmon",
    "Rsvd10",     "Rsvd11",      "Rsvd12",       "Breakpoint",
    "Rsvd14",     "Rsvd15",      "Rsvd16",       "Thermal",
};

void flashDiscLedForever() {
    while(1) {
        SET_DISC_LED(1);
        udelay(100000000);
        SET_DISC_LED(0);
        udelay(100000000);
    }
}

void writeMemDump() {
    if(!crashDump) {
        exiPuts("No crash dump file\n");
        return;
    }
    exiPuts("Writing crash dump (8x)");
    void *addr = (void*)0x80000000;
    for(int i=0; i<24; i++) {
        fwrite(addr, 1024, 1024, crashDump);
        exiPuts(".");
        addr += 1024*1024;
    }
    exiPuts("\r\nWriting crash dump (9x)");
    addr = (void*)0x90000000;
    for(int i=0; i<64; i++) {
        fwrite(addr, 1024, 1024, crashDump);
        exiPuts(".");
        addr += 1024*1024;
    }
    exiPuts("Wrote crash dump OK\r\n");
    fclose(crashDump);
}

void writeCrashLog(int exceptionCode, OSContext *ctx,
uint cause, void *addr, u32 msr) {
    if(!areInterruptsEnabled()) {
        exiPuts("Can't write crash dump during IRQ\r\n");
        return;
    }
    //OSEnableScheduler();
    //OSEnableInterrupts();
    writeMemDump();
    if(!crashLog) return;
    exiPuts("Writing crash log\r\n");
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

void gameExceptionHook(int exceptionCode, OSContext *ctx,
uint cause, void *addr) {
    //if(haveGecko) iguanaSetRedLed(true);
    if(haveGecko) {
        iguanaSetRedLed(true);
        interactiveDebugger(cause);
    }
    static bool alreadyExc = false;
    if(alreadyExc) flashDiscLedForever();
    alreadyExc = true;

    u32 msr = mfmsr();
    //udelay(500000);

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
        strcpy(msg, "XER=........ SRR0=........ [........] SRR1=........ LR=........\n");
        putHex(&msg[ 4], ctx->xer);
        putHex(&msg[18], ctx->srr0);
        if(PTR_VALID(ctx->srr0)) putHex(&msg[28], *(u32*)ctx->srr0);
        putHex(&msg[43], ctx->srr1);
        putHex(&msg[55], ctx->lr);
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
        while(PTR_VALID(sp)) {
            putHex(&msg[3], sp[1]);
            exiPuts(msg);
            sp = (u32*)*sp;
        }
    }

    //writeCrashLog(exceptionCode, ctx, cause, addr, msr);
    //dumpGameHeaps();
    if(areInterruptsEnabled()) checkIntegrity();

    u32 srr0 = ctx->srr0 - 128;
    if(PTR_VALID(srr0)) {
        strcpy(msg, "SRR0=........\n");
        putHex(&msg[5], srr0+128);
        exiPuts(msg);
        strcpy(msg, "........: ........ ........ ........ ........\n");
        for(int i=0; i<32; i++) {
            putHex(&msg[ 0], srr0);
            putHex(&msg[10], *(u32*)srr0);
            putHex(&msg[19], *(u32*)(srr0+4));
            putHex(&msg[28], *(u32*)(srr0+8));
            putHex(&msg[37], *(u32*)(srr0+12));
            srr0 += 16;
            exiPuts(msg);
        }
    }

    //SET_SCREEN_SOLID_YUV(76, 84, 255); //red
    //udelay(500000);
    //exiPuts("Crash log written OK.\r\n");
    //flashDiscLedForever();

    /*if(haveGecko) {
        iguanaSetRedLed(true);
        interactiveDebugger(cause);
    }*/

    exiPuts("Shutting down!\r\n");
    //STM_ShutdownToStandby();
    STM_RebootSystem();
    while(1);
}

void gameBsodHook() {
    //hook the game's BSOD thread.
    //this is used later instead of the default handler hooked
    //by the above function. hooking here instead has the advantage
    //that we aren't running in an IRQ, so can dump to SD card.
    int exceptionCode = *(s16*)0x803dda40;
    OSContext *ctx = *(OSContext**)0x803dda3c;
    uint cause = *(uint*)0x803dda38;
    void *addr = *(void**)0x803dda34;
    gameExceptionHook(exceptionCode, ctx, cause, addr);
}
