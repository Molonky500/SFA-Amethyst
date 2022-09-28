#include "main.h"

mutex_t exiMutex;

void exiPuts(const char *str) {
    /** Send a string to the EXI UART.
     */
    //LWP_MutexLock(exiMutex);
    u32 irq = IRQ_Disable();

    __attribute__ ((aligned (32))) static u8 dmaBuf[4096];
    //0x800400 is address for UART.
    //high bit indicates write.
    static u32 addr = (0x800400 << 6) | 0x80000000;
    static volatile u32 *exi = (volatile u32*)0xCD006800; //channel 0

    ssize_t len = strlen(str);
    while(len > 0) {
        dmaBuf[0] = addr >> 24;
        dmaBuf[1] = addr >> 16;
        dmaBuf[2] = addr >>  8;
        dmaBuf[3] = addr;

        //copy a chunk of the string
        ssize_t copyLen = len;
        if(copyLen >= sizeof(dmaBuf)-4) {
            copyLen = sizeof(dmaBuf)-4;
        }

        //pad the length - must be a multiple of 32 bytes
        ssize_t paddedLen = copyLen;
        ssize_t pad = paddedLen & 0x1F;
        if(pad) paddedLen += (32-pad);

        //copy the string, replacing '\n' with '\n\r'
        //(NOT '\r\n') because dolphin treats '\r' as
        //"actually print the buffered message".
        int outPos=4;
        while(*str && outPos < sizeof(dmaBuf)) {
            char c = *(str++);
            if(c == '\n') {
                dmaBuf[outPos++] = '\n';
                if(outPos < sizeof(dmaBuf)) dmaBuf[outPos++] = '\r';
            }
            else if(c != '\r') dmaBuf[outPos++] = c;
        }
        while(outPos < paddedLen && outPos < sizeof(dmaBuf)) {
            dmaBuf[outPos++] = 0;
        }

        exi[0] = (1 << 13) | //ROMDIS
            (5 << 4) | //32MHz
            (2 << 7); //device 1 (UART)

        exi[1] = ((u32)&dmaBuf) & 0x7FFFFFFF; //DMA source
        exi[2] = outPos; //DMA length
        exi[3] = (1 << 2) | //write
            (1 << 1) | //use DMA
            (1 << 0); //start now

        while(exi[3] & 1); //wait for transfer

        len -= copyLen;
    }
    IRQ_Restore(irq);
    //LWP_MutexUnlock(exiMutex);
}

void exiPrintf(const char *fmt, ...) {
    if(!(get_msr() & 0x8000)) {
        //vsnprintf doesn't work with interrupts disabled.
        exiPuts(fmt);
        return;
    }
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);
    va_end(args);
}

void exiPrintInit() {
    u32 irq = IRQ_Disable();
    LWP_MutexInit(&exiMutex, false);
    volatile u32 *exi = (volatile u32*)0xCD006800; //channel 0
    while(exi[3] & 1); //wait for TSTART
    exi[3] = 0;
    (*(volatile uint32_t*)0xCD00643C) = 0; //enable 32MHz
    IRQ_Restore(irq);
    exiPuts("EXI init OK\n");
}
