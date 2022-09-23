#include "main.h"

void disableIrq() {
    __asm__(
        "mfmsr 3           \n"
        "ori   4, 3, 0x8000\n"
        "mtmsr 4, 0        \n"
    );
}
void enableIrq() {
    __asm__(
        "mfmsr 3           \n"
        "lis   4, 0xFFFF   \n"
        "ori   4, 4, 0x7FFF\n"
        "and   3, 3, 4     \n"
        "mtmsr 3, 0        \n"
    );
}

//XXX not used
void setupIrqHandler() {
    //copy our custom handler over the unused thermal handler
    u32 len = (u32)&_raw_exceptionHook_External_END -
        (u32)_raw_exceptionHook_External;
    memcpy((void*)0x80001700, _raw_exceptionHook_External, len);

    //copy our FP Unavailable handler over empty space
    u32 lenFP = (u32)&_raw_exceptionHook_FpUnavailable_END -
        (u32)_raw_exceptionHook_FpUnavailable;
    memcpy((void*)0x80001600, _raw_exceptionHook_FpUnavailable,
        lenFP);

    //save the OGC regs
    u32 *regs = (u32*)(0x80001700 + len);
    regs[-1] = get_r13();
    regs[-2] = get_r2();

    //patch the default interrupt handler to call ours.
    //we use 8x memory space here even though it executes
    //in 0x space because branches are relative.
    disableIrq();
    (*(u32*)0x80001700) = (*(u32*)0x80000500);
    hookBranch(0x80000500, 0x80001700, 0);
    (*(u32*)0x80001600) = (*(u32*)0x80000800);
    hookBranch(0x80000800, 0x80001600, 0);

    //find the "rfi" instruction
    for(u32 *op = (u32*)0x80000504; op < (u32*)0x80000600; op++) {
        if(*op == 0x4c000064) {
            hookBranch(op,
                (((u32)&_raw_exceptionHook_External2 - (u32)_raw_exceptionHook_External) + 0x1700) | 0x80000000,
                0);
            break;
        }
    }
    for(u32 *op = (u32*)0x80000804; op < (u32*)0x80000900; op++) {
        if(*op == 0x4c000064) {
            //not _raw_exceptionHook_FpUnavailable2 here
            hookBranch(op,
                (((u32)&_raw_exceptionHook_External2 - (u32)_raw_exceptionHook_External) + 0x1700) | 0x80000000,
                0);
            break;
        }
    }
    DCInvalidateRange((void*)0x80000100, 0x1800-0x0100);
    for(u32 addr=0x80000100; addr < 0x80001800; addr += 32) {
        ICBlockInvalidate(addr);
    }
    enableIrq();
}

extern void _cpu_context_switch(void *,void *);
extern void _cpu_context_switch_ex(void *,void *);
extern void _cpu_context_save(void *);
extern void _cpu_context_restore(void *);
extern void _cpu_context_save_fp(void *);
extern void _cpu_context_restore_fp(void *);
extern vu32 _context_switch_want;
extern vu32 _thread_dispatch_disable_level;

void gameIrqHandler(u32 irq, void *ctx) {
    //irq is the IRQ number
    //ctx is the OSContext we passed to set up this IRQ
    //switchToOgc();
    //u32 prev = IRQ_Disable();

    //frame_context sctx;
    //_cpu_context_save(&sctx);
    //_cpu_context_save_fp(&sctx);

    // *(vu32*)0x803dde88 = 1;
    //vu32 oldC = _context_switch_want;
    //_context_switch_want = 0;
    //vu32 oldD = _thread_dispatch_disable_level;
    //_thread_dispatch_disable_level = 1;
    static u32 *handlers = (u32*)0x80003040;
    void (*handler)(int, OSContext*) =
    (void (*)(int, OSContext*))handlers[irq];

    //if(irq == 17) { //GX CP
    /*if(irq != 24) { //VI
        char msg[1024];
        strcpy(msg, "game irq ........ => ........ th ........\n");
        putHex(&msg[ 9], irq);
        putHex(&msg[21], (u32)handler);
        putHex(&msg[33], (u32)LWP_GetSelf());
        exiPuts(msg);
    }*/

    //call game's original handler
    switchToGame();
    //IRQ_Restore(prev);
    if(handler) handler(irq, (OSContext*)ctx);

    switchToOgc();
    //_context_switch_want = oldC;
    //_thread_dispatch_disable_level = oldD;
    //_cpu_context_restore_fp(&sctx);
    //_cpu_context_restore(&sctx);
    //LWP_YieldThread();
}
