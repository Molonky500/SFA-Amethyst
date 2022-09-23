#include "main.h"

static vu32 ogc_r2   = 0xDEADBEEF;
static vu32 ogc_r13  = 0xDEADBEEF;
static vu32 game_r2  = 0x803E6500;
static vu32 game_r13 = 0x803E31E0;

void switchToGame() {
    u32 irq = IRQ_Disable();
    ogc_r2 = get_r2();
    set_r2(game_r2);
    ogc_r13 = get_r13();
    set_r13(game_r13);
    //set_msr(0x00009032);
    IRQ_Restore(irq);
}
void switchToOgc() {
    u32 irq = IRQ_Disable();
    //game ones never change, no need to save
    set_r2(ogc_r2);
    set_r13(ogc_r13);
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

void exceptionHook(u32 *exc_gpr, int code) {
    static bool alreadyExc = false;
    while(alreadyExc);
    alreadyExc = true;

    char msg[1024];
    strcpy(msg, "ERR ........\n");
    putHex(&msg[4], code);
    exiPuts(msg);

    //.set LR_SAVE,    0x00
    //.set XER_SAVE,   0x04
    //.set SRR0_SAVE,  0x08
    //.set SRR1_SAVE,  0x0C
    //.set GPR_SAVE,   0x10

    strcpy(msg, "XER=........ SRR0=........ SRR1=........ LR=........\n");
    putHex(&msg[ 4], exc_gpr[0x04/4]);
    putHex(&msg[18], exc_gpr[0x08/4]);
    putHex(&msg[32], exc_gpr[0x0C/4]);
    putHex(&msg[44], exc_gpr[0x00/4]);
    exiPuts(msg);

    exiPuts("GPRs:\n");
    u32 *gpr = &exc_gpr[0x10/4];
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

void c_exception_handler(frame_context *pCtx) {
    exceptionHook(pCtx->GPR, pCtx->EXCPT_Number);
}
