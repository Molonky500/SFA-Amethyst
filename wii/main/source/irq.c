#include "main.h"

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
