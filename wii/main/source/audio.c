#include "main.h"
#define AUDIO_DEBUG 0

//this is probably not needed
//but might be, because of different masking

void ARStartDMA_hook(u32 type, void *mmaddr, u32 araddr, u32 cntL) {
    static int nDma = 0;
    //if(nDma >= 4400) {
    //    return;
    //}
    nDma++;
    //exiPrintf("AR DMA %08X %08X %08X %08X\n",
    //    type, mmaddr, araddr, cntL);

    u32 count = cntL;
    u32 mmEnd = (u32)mmaddr + count;
    u32 arEnd = (u32)araddr + count;
    bool dir  = type != 0; //true: ARAM -> MRAM
    arEnd &= 0x00FFFFFF;

    #if AUDIO_DEBUG
        exiPrintf("AR DMA[%8d]: [%08X, %08X] %s [%08X, %08X] (len=%d)\n",
            nDma, mmaddr, mmEnd, dir ? "<-" : "->", araddr, arEnd, count);
    #endif

    if(mmEnd > 0x81800000 || arEnd > 0x01000000
    || (u32)mmaddr < 0x80000000) {
        exiPuts(" *** ERROR *** AR DMA out of range!\n");
        return;
    }

    #if AUDIO_DEBUG
        if(!checkAddrInheap(mmaddr, count)) {
            exiPrintf(" for AR DMA: [%08X, %08X] %s [%08X, %08X] (len=%d) @%08X < %08X < %08X\n",
                mmaddr, mmEnd, dir ? "<-" : "->", araddr, arEnd, count,
                __builtin_extract_return_addr(__builtin_return_address(0)),
                __builtin_extract_return_addr(__builtin_return_address(1)),
                __builtin_extract_return_addr(__builtin_return_address(2))
            );
            return;
        }
    #endif

    u32 level = IRQ_Disable();
    u32 wVar1 = AR_DMA_MMADDR_H;
    AR_DMA_MMADDR_H = wVar1 & 0xfc00 | (ushort)((uint)mmaddr >> 0x10);
    wVar1 = AR_DMA_MMADDR_L;
    AR_DMA_MMADDR_L = wVar1 & 0x1f | (ushort)mmaddr;
    wVar1 = AR_DMA_ARADDR_H;
    AR_DMA_ARADDR_H = wVar1 & 0xfc00 | (ushort)((uint)araddr >> 0x10);
    wVar1 = AR_DMA_ARADDR_L;
    AR_DMA_ARADDR_L = wVar1 & 0x1f | (ushort)araddr;
    wVar1 = AR_DMA_CNT_H;
    AR_DMA_CNT_H = (ushort)(type << 0xf) | wVar1 & 0x7fff;
    wVar1 = AR_DMA_CNT_H;
    AR_DMA_CNT_H = wVar1 & 0xfc00 | (ushort)((uint)cntL >> 0x10);
    wVar1 = AR_DMA_CNT_L;
    AR_DMA_CNT_L = wVar1 & 0x1f | (ushort)cntL;
    //while(AR_DMA_CNT_LEFT);

    IRQ_Restore(level);
}

void AIInitDMA_hook(u32 start, uint length) {
    static int nDma = 0;
    nDma++;
    #if AUDIO_DEBUG
        exiPrintf("AI DMA[%8d] %08X %08X\n", nDma, start, length);
    #endif
    // For AUDIO_DMA_START_HI, only bits 0x03ff can be set on
    // GCN and 0x1fff on Wii
    // For AUDIO_DMA_START_LO, only bits 0xffe0 can be set
    u32 level = IRQ_Disable();
    AI_DMA_START_HI = (AI_DMA_START_HI & 0xfc00) | ((ushort)((uint)start >> 0x10) & 0x03FF);
    AI_DMA_START_LO = (AI_DMA_START_LO & 0x001f) | (ushort)start;
    AI_DMA_LENGTH   = (AI_DMA_LENGTH   & 0x8000) | (ushort)(length >> 5);
    IRQ_Restore(level);
}
