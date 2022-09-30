#include "main.h"
#define AUDIO_DEBUG 0

//this is probably not needed
//but might be, because of different masking

void ARStartDMA_hook(u32 cntH, void *mmaddr, u32 araddr, u32 cntL) {
    static int nDma = 0;
    //if(nDma >= 4400) {
    //    return;
    //}
    nDma++;

    u32 count = (cntH << 16) | cntL;
    u32 mmEnd = (u32)mmaddr + count;
    u32 arEnd = (u32)araddr + count;
    bool dir  = (cntH & 0x8000); //true: ARAM -> MRAM

    #if AUDIO_DEBUG
        exiPrintf("AR DMA[%8d]: [%08X, %08X] %s [%08X, %08X] (len=%d)\n",
            nDma, mmaddr, mmEnd, dir ? "<-" : "->", araddr, arEnd, count);
    #endif

    if(mmEnd > 0x81800000 || arEnd > 0x01000000
    || (u32)mmaddr < 0x80000000) {
        exiPuts(" *** ERROR *** AR DMA out of range!\n");
        return;
    }
    if(!checkAddrInheap(mmaddr, count)) {
        exiPrintf(" for AR DMA: [%08X, %08X] %s [%08X, %08X] (len=%d) @%08X\n",
            mmaddr, mmEnd, dir ? "<-" : "->", araddr, arEnd, count,
            __builtin_extract_return_addr(__builtin_return_address(0)));
        return;
    }

    u32 level = IRQ_Disable();
    AR_DMA_MMADDR_H = (AR_DMA_MMADDR_H & 0xfc00) | ((uint)mmaddr >> 0x10);
    AR_DMA_MMADDR_L = (AR_DMA_MMADDR_L & 0x001f) | (ushort)(u32)mmaddr;
    AR_DMA_ARADDR_H = (AR_DMA_ARADDR_H & 0xfc00) | ((uint)araddr >> 0x10);
    AR_DMA_ARADDR_L = (AR_DMA_ARADDR_L & 0x001f) | (ushort)araddr;

    u32 ch = ((cntH << 0xF) | (cntL >> 0x10)) & 0x03FF;
    AR_DMA_CNT_H    = (AR_DMA_CNT_H & 0x7fff) | ch;
    AR_DMA_CNT_L    = (AR_DMA_CNT_L & 0x001f) | (ushort)cntL;
    //while(AR_DMA_CNT_LEFT);
    /*if(dir) {
        memcpy(mmaddr, araddr | 0x90000000, count);
    }
    else {
        memcpy(araddr | 0x90000000, mmaddr, count);
    }*/
    //*(u8*)0x803d41e1 = 0; //DMA complete

    //this is probably causing stack overflow because the
    //handler tries to do more AR DMA
    /*u32 lol = *(u32*)0x803de018;
    void (*irqHandler)(void) = (void (*)())lol;
    if(irqHandler) {
        irqHandler();
    }*/

    IRQ_Restore(level);
}

void AIInitDMA_hook(u32 start, uint length) {
    static int nDma = 0;
    nDma++;
    #if AUDIO_DEBUG
        exiPrintf("AI DMA[%8d] %08X %08X\n", nDma, start, length);
    #endif
    u32 level = IRQ_Disable();
    AI_DMA_START_HI = (AI_DMA_START_HI & 0xfc00) | ((ushort)((uint)start >> 0x10) & 0x03FF);
    AI_DMA_START_LO = (AI_DMA_START_LO & 0x001f) | (ushort)start;
    AI_DMA_LENGTH   = (AI_DMA_LENGTH   & 0x8000) | (ushort)(length >> 5);
    IRQ_Restore(level);
}
