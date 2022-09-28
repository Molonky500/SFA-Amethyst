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
void osPrintHook(const char *fmt, ...) {
    switchToOgc();
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);

    int len = strlen(buf);
    if(buf[len-1] != '\n') exiPuts("\n");
    va_end(args);
    switchToGame();
}
