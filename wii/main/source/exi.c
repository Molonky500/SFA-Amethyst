#include "main.h"

//mutex_t exiMutex;
__attribute__ ((aligned (32))) u8 exiDmaBuf[4096];
u8 debugDeviceType = DBG_DEV_NONE;

u32 exiReadWrite32(volatile u32 *exi, u32 val) {
    exi[4] = val;
    exi[3] = (2 << 2) | //read and write
        (3 << 4) | //4 bytes
        (1 << 0); //start now
    while(exi[3] & 1); //wait for any previous transfer
    return exi[4];
}

void exiPuts(const char *str) {
    /** Send a string to the EXI UART.
     */
    //LWP_MutexLock(exiMutex);
    if(debugDeviceType == DBG_DEV_IGUANA) {
        iguanaPutsNoFlush(str);
        return;
    }
    else if(debugDeviceType == DBG_DEV_NONE) return;

    u32 irq = IRQ_Disable();
    memset(exiDmaBuf, 0, sizeof(exiDmaBuf));

    static volatile u32 *exi = (volatile u32*)0xCD006800; //channel 0

    //skip any leftover line breaks and this \x01 that
    //keeps somehow sneaking in.
    //while(*str == '\r' || *str == '\n' || *str == '\x01') str++;

    ssize_t len = strlen(str);
    while(len > 0 && *str) {
        int outPos=0;

        //0x800400 is address for UART.
        //high bit indicates write.
        static u32 addr = (0x800400 << 6) | 0x80000000;
        exiDmaBuf[outPos++] = addr >> 24;
        exiDmaBuf[outPos++] = addr >> 16;
        exiDmaBuf[outPos++] = addr >>  8;
        exiDmaBuf[outPos++] = addr;

        //copy a chunk of the string
        ssize_t copyLen = len;
        if(copyLen >= sizeof(exiDmaBuf)-4) {
            copyLen = sizeof(exiDmaBuf)-4;
        }

        //pad the length - must be a multiple of 32 bytes
        ssize_t paddedLen = copyLen;
        ssize_t pad = paddedLen & 0x1F;
        if(pad) paddedLen += (32-pad);

        //copy the string, replacing '\n' with '\r'
        //because dolphin (and probably real HW) treats '\r' as
        //"actually print the buffered message".

        while(*str && outPos < sizeof(exiDmaBuf)) {
            char c = *(str++);
            if(c == '\n') {
                exiDmaBuf[outPos++] = '\r';
                while((*str == '\r') || (*str == '\n')
                || (*str == '\x01')) str++;
            }
            else if(c != '\r' && c != '\x01') exiDmaBuf[outPos++] = c;
        }
        while(outPos < paddedLen && outPos < sizeof(exiDmaBuf)) {
            exiDmaBuf[outPos++] = 0;
        }
        DCFlushRange(exiDmaBuf, outPos);

        while(exi[3] & 1); //wait for any previous transfer
        u32 prev0 = exi[0];
        //u32 prev1 = exi[1];
        //u32 prev2 = exi[2];
        exi[0] =
            (1 << 13) | //ROMDIS
            //(5 <<  4) | //32MHz
            (0 <<  4) | //1MHz
            //(4 <<  4) | //16MHz
            (2 <<  7); //device 1 (UART)
        #if 1
            exi[1] = MEM_VIRTUAL_TO_PHYSICAL(exiDmaBuf); //DMA source
            exi[2] = (outPos & ~0x1F); //DMA length
            exi[3] = (1 << 2) | //write
                (1 << 1) | //use DMA
                (1 << 0); //start now
            while(exi[3] & 1); //wait for transfer
            //exi[0] |= (1 << 1) | (1 << 11); //clear interrupt flags
            //exi[2] = prev2;
            //exi[1] = prev1;
        #else
            SET_DEBUG_PORT(0xFF); udelay(10000);
            for(int i=0; i<outPos; i++) {
                SET_DEBUG_PORT(exiDmaBuf[i]); udelay(10000);
                exi[5] = exiDmaBuf[i];
                exi[4] = (1 << 2); //write 1 byte immediate
                while(exi[3] & 1); //wait for transfer
            }
            SET_DEBUG_PORT(0xEE); udelay(10000);
        #endif
        exi[0] = prev0;
        len -= copyLen;
    }
    //SET_DEBUG_PORT(0xEA);
    IRQ_Restore(irq);
    //LWP_MutexUnlock(exiMutex);
}

void exiPrintf(const char *fmt, ...) {
    //our printf() eventually just calls exiPuts()
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void exiPrintInit() {
    //SET_DEBUG_PORT(0xD1);
    u32 irq = IRQ_Disable();
    //LWP_MutexInit(&exiMutex, false);
    //_ipcReg[0x70>>2] |= 1; //enable EXI (makes no difference)
    while(_exiReg[3] & 1); //wait for TSTART
    _exiReg[3] = 0;
    //(*(volatile uint32_t*)0xCD00643C) = 0; //enable 32MHz

    iguanaInit();

    //SET_DEBUG_PORT(0xD2);
    IRQ_Restore(irq);
    exiPuts("EXI init OK\r\n");
    exiPuts("The message \"Unknown USBGecko command 1230abc\" can be safely ignored.\r\n");
}

void exiInterrupt_hook() {
    //only triggers when debug adapter asserts INT.
    SET_DEBUG_PORT(0xEE);
    exiPuts(" *** EXI INTERRUPT\n");

    /*dumpThreads();
    dvdDumpPendingReadCallbacks();
    dvdDumpPendingCancelCallbacks();
    dvdDumpPendingStreamCallbacks();
    dvdDumpOpenFiles();
    writeMemDump();*/
    interactiveDebugger(5);
}
