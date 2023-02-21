#include "main.h"

//mutex_t exiMutex;
__attribute__ ((aligned (32))) static u8 dmaBuf[4096];

void exiPuts(const char *str) {
    /** Send a string to the EXI UART.
     */
    //LWP_MutexLock(exiMutex);
    u32 irq = IRQ_Disable();
    memset(dmaBuf, 0, sizeof(dmaBuf));

    //0x800400 is address for UART.
    //high bit indicates write.
    #if USE_CUSTOM_GECKO
        static volatile u32 *exi = (volatile u32*)0xCD006814; //channel 1
    #else
        static volatile u32 *exi = (volatile u32*)0xCD006800; //channel 0
    #endif

    //skip any leftover line breaks and this \x01 that
    //keeps somehow sneaking in.
    //while(*str == '\r' || *str == '\n' || *str == '\x01') str++;

    ssize_t len = strlen(str);
    while(len > 0 && *str) {
        int outPos=0;
        #if USE_CUSTOM_GECKO
            dmaBuf[outPos++] = 0xDE;
            dmaBuf[outPos++] = 0xAD;
            dmaBuf[outPos++] = 0xFA;
            dmaBuf[outPos++] = 0xCE;
        #else
            static u32 addr = (0x800400 << 6) | 0x80000000;
            dmaBuf[outPos++] = addr >> 24;
            dmaBuf[outPos++] = addr >> 16;
            dmaBuf[outPos++] = addr >>  8;
            dmaBuf[outPos++] = addr;
        #endif

        //copy a chunk of the string
        ssize_t copyLen = len;
        #if USE_CUSTOM_GECKO
            copyLen += 7;
        #endif
        if(copyLen >= sizeof(dmaBuf)-4) {
            copyLen = sizeof(dmaBuf)-4;
        }

        //pad the length - must be a multiple of 32 bytes
        ssize_t paddedLen = copyLen;
        ssize_t pad = paddedLen & 0x1F;
        if(pad) paddedLen += (32-pad);

        #if USE_CUSTOM_GECKO
            uint32_t nBlocks = paddedLen >> 5;
            dmaBuf[outPos++] = (nBlocks >> 8) & 0xFF;
            dmaBuf[outPos++] = (nBlocks >> 0) & 0xFF;
        #endif

        //copy the string, replacing '\n' with '\r'
        //because dolphin (and probably real HW) treats '\r' as
        //"actually print the buffered message".
        #if USE_CUSTOM_GECKO
            while(*str && outPos < sizeof(dmaBuf)) {
                char c = *(str++);
                if(c == '\n') dmaBuf[outPos++] = '\r';
                dmaBuf[outPos++] = c;
            }
        #else
            while(*str && outPos < sizeof(dmaBuf)) {
                char c = *(str++);
                if(c == '\n') {
                    dmaBuf[outPos++] = '\r';
                    while((*str == '\r') || (*str == '\n')
                    || (*str == '\x01')) str++;
                }
                else if(c != '\r' && c != '\x01') dmaBuf[outPos++] = c;
            }
        #endif
        while(outPos < paddedLen && outPos < sizeof(dmaBuf)) {
            dmaBuf[outPos++] = 0;
        }

        #if USE_CUSTOM_GECKO
            //send one more block than needed so that the device has a
            //chance to read in all of the message, even if it has to
            //skip past a few padding bytes first.
            outPos += 32;
        #endif
        DCFlushRange(dmaBuf, outPos);

        while(exi[3] & 1); //wait for any previous transfer
        u32 prev0 = exi[0];
        //u32 prev1 = exi[1];
        //u32 prev2 = exi[2];
        exi[0] =
            (1 << 13) | //ROMDIS
            //(5 <<  4) | //32MHz
            (0 <<  4) | //1MHz
            //(4 <<  4) | //16MHz
        #if USE_CUSTOM_GECKO
            (1 <<  7); //device 0 (Gecko)
        #else
            (2 <<  7); //device 1 (UART)
        #endif
        exi[1] = MEM_VIRTUAL_TO_PHYSICAL(dmaBuf); //DMA source
        exi[2] = (outPos & ~0x1F); //DMA length
        exi[3] = (1 << 2) | //write
            (1 << 1) | //use DMA
            (1 << 0); //start now
        while(exi[3] & 1); //wait for transfer
        //exi[0] |= (1 << 1) | (1 << 11); //clear interrupt flags
        //exi[2] = prev2;
        //exi[1] = prev1;
        exi[0] = prev0;
        len -= copyLen;
    }
    IRQ_Restore(irq);
    //LWP_MutexUnlock(exiMutex);
}

void exiPrintf(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);
    va_end(args);
}

void exiPrintInit() {
    u32 irq = IRQ_Disable();
    //LWP_MutexInit(&exiMutex, false);
    //XXX fishy constant
    //_exiReg[0] = (vu32)0xCD006800; //channel 0
    while(_exiReg[3] & 1); //wait for TSTART
    _exiReg[3] = 0;
    //(*(volatile uint32_t*)0xCD00643C) = 0; //enable 32MHz

    //enable INT interrupt for custom Gecko thing.
    static volatile u32 *exi = (volatile u32*)0xCD006814; //channel 1
    exi[0] |= (1 << 0);

    IRQ_Restore(irq);
    exiPuts("EXI init OK\r\n");
}

void exiInterrupt_hook() {
    //only triggers when debug adapter asserts INT.
    SET_DEBUG_PORT(0xEE);
    exiPuts(" *** EXI INTERRUPT\n");

    //dump threads
    OSThread *thread = *(OSThread**)0x800000dc;
    while(thread) {
        u32 stackTop = (u32)thread->stackBase; //high addr
        u32 stackBot = (u32)thread->stackEnd;  //low  addr
        u32 *sp      = (u32*)thread->context.gpr[1];
        u32 pc       = (u32)thread->context.srr0;

        const char *name = "unknown";
        PrevThreadState *pState = findThread(thread);
        if(pState) {
            if(pState->name != NULL) name = pState->name;
        }
        exiPrintf("Thread %08X (%s): PC=%08x\n", thread, name, pc);
        for(int i=0; i<32; i += 4) {
            for(int j=0; j<4; j++) {
                exiPrintf("  r%2d: %08x", (i+j), (u32)thread->context.gpr[i+j]);
            }
            exiPrintf("\n");
        }
        while(PTR_VALID(sp)) {
            exiPrintf(" > %08x\n", sp[1]);
            sp = (u32*)*sp;
        }
        thread = thread->linkActive.next;
    }
    dvdDumpPendingReadCallbacks();
    dvdDumpPendingCancelCallbacks();
    dvdDumpPendingStreamCallbacks();
    dvdDumpOpenFiles();
    writeMemDump();
}
