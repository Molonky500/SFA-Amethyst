#include "main.h"
#define AUDIO_DEBUG 1

#define AX_DSP_NO_DUP3 (0xFFFF)
#define R8(a) (*(u8*)(a))
#define R16(a) (*(u16*)(a))
#define R32(a) (*(u32*)(a))
#define W8(a, v) (*(u8*)(a) = (v))
#define W16(a, v) (*(u16*)(a) = (v))
#define W32(a, v) (*(u32*)(a) = (v))

static void PatchAX_Dsp(u32 ptr, u32 Dup1, u32 Dup2, u32 Dup3,
u32 Dup2Offset) {
    //from Nintendont. not entirely sure what it does, but
    //it makes the sound work correctly.
	static const u32 MoveLength = 0x22;
	static const u32 CopyLength = 0x12;
	static const u32 CallLength = 0x2 + 0x2; // call (2) ; jmp (2)
	static const u32 New1Length = 0x1 * 3 + 0x2 + 0x7; // 3 * orc (1) ; jmp (2) ; patch (7)
	static const u32 New2Length = 0x1; // ret
	u32 SourceAddr = Dup1 + MoveLength;
	u32 DestAddr = Dup2 + CallLength + CopyLength + New2Length;
	u32 Dup2PAddr = DestAddr; // Dup2+0x17 (0xB Patch fits before +0x22)
	u32 Tmp;

	DestAddr--;
	W16((u32)ptr + (DestAddr)* 2, 0x02DF);  // ret
	while (SourceAddr >= Dup1 + 1)  // ensure include Dup1 below
	{
		SourceAddr -= 2;
		Tmp = R32((u32)ptr + (SourceAddr)* 2); // original instructions
		if ((Tmp & 0xFFFFFFFF) == 0x195E2ED1) // lrri        $AC0.M srs         @SampleFormat, $AC0.M
		{
			DestAddr -= 7;
			W32((u32)ptr + (DestAddr + 0x0) * 2, 0x03400003); // andi        $AC1.M, #0x0003
			W32((u32)ptr + (DestAddr + 0x2) * 2, 0x8100009E); // clr         $ACC0 -
			W32((u32)ptr + (DestAddr + 0x4) * 2, 0x200002CA); // lri         $AC0.M, 0x2000 lsrn
			W16((u32)ptr + (DestAddr + 0x6) * 2, 0x1FFE);     // mrr     $AC1.M, $AC0.M
			Tmp = Tmp | 0x00010100; // lrri        $AC1.M srs         @SampleFormat, $AC1.M
		}
		DestAddr -= 2;
		W32((u32)ptr + (DestAddr)* 2, Tmp); // unchanged
		if ((Tmp & 0x0000FFF1) == 0x00002ED0) // srs @ACxAH, $AC0.M
		{
			DestAddr -= 1;
			W32((u32)ptr + (DestAddr)* 2, (Tmp & 0xFFFF0000) | 0x3E00); //  orc AC0.M AC1.M
		}
		if (DestAddr == Dup2 + CallLength) // CopyLength must be even
		{
			DestAddr = Dup1 + CallLength + MoveLength - CopyLength + New1Length;
			DestAddr -= 2;
			W32((u32)ptr + (DestAddr)* 2, 0x029F0000 | (Dup2 + CallLength)); // jmp Dup2+4
		}
	}
	W32((u32)ptr + (Dup1 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
	W32((u32)ptr + (Dup1 + 0x02) * 2, 0x029F0000 | (Dup1 + MoveLength)); // jmp Dup1+0x22
	W32((u32)ptr + (Dup2 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
	W32((u32)ptr + (Dup2 + 0x02) * 2, 0x029F0000 | (Dup2 + MoveLength)); // jmp Dup2+0x22
	Tmp = R32((u32)ptr + (Dup1 + 0x98) * 2); // original instructions
	W32((u32)ptr + (Dup1 + 0x98) * 2, 0x02BF0000 | (Dup2PAddr)); // call Dup2PAddr
	W32((u32)ptr + (Dup2 + Dup2Offset) * 2, 0x02BF0000 | (Dup2PAddr)); // call Dup2PAddr

	W16((u32)ptr + (Dup2PAddr + 0x00) * 2, Tmp >> 16); //  original instructions (Start of Dup2PAddr [0xB long])
	W32((u32)ptr + (Dup2PAddr + 0x01) * 2, 0x27D10340); // lrs         $AC1.M, @SampleFormat -
	W32((u32)ptr + (Dup2PAddr + 0x03) * 2, 0x00038100); // andi        $AC1.M, #0x0003 clr         $ACC0
	W32((u32)ptr + (Dup2PAddr + 0x05) * 2, 0x009E1FFF); // lri         $AC0.M, #0x1FFF
	W16((u32)ptr + (Dup2PAddr + 0x07) * 2, 0x02CA);     // lsrn
	W16((u32)ptr + (Dup2PAddr + 0x08) * 2, Tmp & 0xFFFF); //  original instructions
	W32((u32)ptr + (Dup2PAddr + 0x09) * 2, 0x3D0002DF); // andc        $AC1.M, $AC0.M ret

	if (Dup3 != AX_DSP_NO_DUP3)
	{
		W32((u32)ptr + (Dup3 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
		W32((u32)ptr + (Dup3 + 0x02) * 2, 0x029F0000 | (Dup3 + MoveLength)); // jmp Dup3+0x22

		W32((u32)ptr + (Dup3 + 0x04) * 2, 0x27D10340); // lrs         $AC1.M, @SampleFormat -
		W32((u32)ptr + (Dup3 + 0x06) * 2, 0x00038100); // andi        $AC1.M, #0x0003 clr         $ACC0
		W32((u32)ptr + (Dup3 + 0x08) * 2, 0x009E1FFF); // lri         $AC0.M, #0x1FFF
		W16((u32)ptr + (Dup3 + 0x0A) * 2, 0x02CA);     // lsrn
		Tmp = R32((u32)ptr + (Dup3 + 0x5D) * 2); // original instructions
		W16((u32)ptr + (Dup3 + 0x0B) * 2, Tmp >> 16); //  original instructions
		W16((u32)ptr + (Dup3 + 0x0C) * 2, 0x3D00); // andc        $AC1.M, $AC0.M
		W16((u32)ptr + (Dup3 + 0x0D) * 2, Tmp & 0xFFFF); //  original instructions
		Tmp = R32((u32)ptr + (Dup3 + 0x5F) * 2); // original instructions
		W32((u32)ptr + (Dup3 + 0x0E) * 2, Tmp); //  original instructions includes ret

		W32((u32)ptr + (Dup3 + 0x5D) * 2, 0x029F0000 | (Dup3 + CallLength)); // jmp Dup3+0x4
	}
	return;
}

void doDspPatch() {
    PatchAX_Dsp(0x80330840, 0x5A8, 0x65D, 0x707, 0x8F );
    DCStoreRange(0x80330840, 0x2000);
}

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
    //arEnd &= 0x00FFFFFF;

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
    /* u32 wVar1 = AR_DMA_MMADDR_H;
    AR_DMA_MMADDR_H = (wVar1 & 0xfc00) | (ushort)((uint)mmaddr >> 0x10);
    wVar1 = AR_DMA_MMADDR_L;
    AR_DMA_MMADDR_L = (wVar1 & 0x1f) | (ushort)mmaddr;
    wVar1 = AR_DMA_ARADDR_H;
    AR_DMA_ARADDR_H = (wVar1 & 0xfc00) | (ushort)((uint)araddr >> 0x10);
    wVar1 = AR_DMA_ARADDR_L;
    AR_DMA_ARADDR_L = (wVar1 & 0x1f) | (ushort)araddr;
    wVar1 = AR_DMA_CNT_H;
    AR_DMA_CNT_H = (ushort)(type << 0xf) | (wVar1 & 0x7fff);
    wVar1 = AR_DMA_CNT_H;
    AR_DMA_CNT_H = (wVar1 & 0xfc00) | (ushort)((uint)cntL >> 0x10);
    wVar1 = AR_DMA_CNT_L;
    AR_DMA_CNT_L = (wVar1 & 0x1f) | (ushort)cntL;
    //while(AR_DMA_CNT_LEFT); */

    araddr += 0x90000000;
    if(dir) { //ARAM -> MRAM
        DCStoreRange(araddr, count);
        memcpy(mmaddr, araddr, count);
        DCStoreRange(mmaddr, count);
    }
    else {
        DCStoreRange(mmaddr, count);
        memcpy(araddr, mmaddr, count);
        DCStoreRange(araddr, count);
    }
    AR_DMA_CNT_H = 0;
    AR_DMA_CNT_L = 0; //trigger completion IRQ

    IRQ_Restore(level);
}

void AIInitDMA_hook(u32 start, uint length) {
    static int nDma = 0;
    nDma++;
    #if AUDIO_DEBUG
        //exiPrintf("AI DMA[%8d] %08X %08X\n", nDma, start, length);
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

void AIStartDMA_hook() {
    //u32 irq = OSDisableInterrupts();
    AI_DMA_LENGTH |= 0x8000;
    //while(AI_DMA_CNT_LEFT);
    //OSRestoreInterrupts(irq);
}
