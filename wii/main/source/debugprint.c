#include "main.h"

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

//same hook is used for several debug print functions
//that are stubbed in the game binary.
//XXX any reason we can't do this on the GC side instead?
//only the lack of vsnprintf which we could port?
void osPrintHook(const char *fmt, ...) {
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);

    int len = strlen(buf);
    if(buf[len-1] != '\n') exiPuts("\r\n");
    va_end(args);
}

void dumpMem(void *addr, uint32_t count) {
    uint32_t *dumpCode = (uint32_t*)addr;
    for(uint32_t i=0; i<count / 4; i++) {
        if((i&3)==0) exiPrintf("%08X: ", (uint32_t)dumpCode);
        exiPrintf("%08X%s", *(dumpCode++),
            ((i&3)==3) ? "\r\n" : " ");
    }
}

void dumpStack() {
    char msg[1024];
    strcpy(msg, "LR = ........ F = ........\r\n");
    putHex(&msg[ 5], __builtin_return_address(0));
    putHex(&msg[18], __builtin_frame_address(0)); //stack pointer
    exiPuts(msg);
    strcpy(msg, "-> ........ [........]\r\n");

    u32 *sp = (u32*)__builtin_frame_address(0);
    while((u32)sp >= 0x80000000 && (u32)sp <= 0x93FFFFFF) {
        putHex(&msg[ 3], sp[1]);
        putHex(&msg[13], sp);
        exiPuts(msg);
        sp = (u32*)*sp;
    }
    exiPuts("-- end\r\n");
}
