#include "main.h"

void osPrintHook(const char *fmt, ...) {
    if(!(get_msr() & 0x8000)) {
        //vsnprintf doesn't work with interrupts disabled.
        exiPuts(fmt);
        return;
    }

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
