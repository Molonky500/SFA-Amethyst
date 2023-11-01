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

//this is here because it involves DSP in a way I don't quite understand
void doCardPatch() {
    //copied from libogc
    static u32 _cardunlockdata[] ATTRIBUTE_ALIGN(32) = {
        0x00000000,0x00000000,0x00000000,0x00000000,
        0x00000000,0x00000000,0x00000021,0x02ff0021,
        0x13061203,0x12041305,0x009200ff,0x0088ffff,
        0x0089ffff,0x008affff,0x008bffff,0x8f0002bf,
        0x008816fc,0xdcd116fd,0x000016fb,0x000102bf,
        0x008e25ff,0x0380ff00,0x02940027,0x02bf008e,
        0x1fdf24ff,0x02403fff,0x00980400,0x009a0010,
        0x00990000,0x8e0002bf,0x009402bf,0x864402bf,
        0x008816fc,0xdcd116fd,0x000316fb,0x00018f00,
        0x02bf008e,0x0380cdd1,0x02940048,0x27ff0380,
        0x00010295,0x005a0380,0x00020295,0x8000029f,
        0x00480021,0x8e0002bf,0x008e25ff,0x02bf008e,
        0x25ff02bf,0x008e25ff,0x02bf008e,0x00c5ffff,
        0x03403fff,0x1c9f02bf,0x008e00c7,0xffff02bf,
        0x008e00c6,0xffff02bf,0x008e00c0,0xffff02bf,
        0x008e20ff,0x03403fff,0x1f5f02bf,0x008e21ff,
        0x02bf008e,0x23ff1205,0x1206029f,0x80b50021,
        0x27fc03c0,0x8000029d,0x008802df,0x27fe03c0,
        0x8000029c,0x008e02df,0x2ece2ccf,0x00f8ffcd,
        0x00f9ffc9,0x00faffcb,0x26c902c0,0x0004029d,
        0x009c02df,0x00000000,0x00000000,0x00000000,
        0x00000000,0x00000000,0x00000000,0x00000000
    };
    memcpy(0x8032ebe0, _cardunlockdata, 0x160);
    DCFlushRange(0x8032ebe0, 0x160);
}

void doDspPatch() {
	//exiPuts(" *** WRITING DSP PATCH\n");
    PatchAX_Dsp(0x80330840, 0x5A8, 0x65D, 0x707, 0x8F );
    DCStoreRange(0x80330840, 0x2000);
	//doCardPatch();
	//exiPuts(" *** WROTE DSP PATCH\n");
}

void ARStartDMA_Hook(int type, u32 mmaddr, u32 araddr, u32 cntL) {
	//type: 0: RAM -> ARAM; 1: ARAM -> RAM
	//exiPrintf("AR DMA RAM:0x%8x %c ARAM:0x%8x len 0x%8x\n", mmaddr,
	//	type ? '<' : '>', araddr, cntL);
	//dumpStack();
	int level = OSDisableInterrupts();
	if(type) { //ARAM to main
		DCInvalidateRange((void*)mmaddr, cntL);
		DCFlushRange((void*)(araddr+0x90000000), cntL);
		memcpy((void*)mmaddr, (void*)(araddr+0x90000000), cntL);
		DCFlushRange((void*)mmaddr, cntL);
	}
	else { //main to ARAM
		DCFlushRange((void*)mmaddr, cntL);
		DCInvalidateRange((void*)(araddr+0x90000000), cntL);
		memcpy((void*)(araddr+0x90000000), (void*)mmaddr, cntL);
		DCFlushRange((void*)(araddr+0x90000000), cntL);
	}
	//we still have to set the registers even though it seems
	//like they no longer do the actual transfer, since the game
	//still expects the interrupt.
	AR_DMA_MMADDR_H = (AR_DMA_MMADDR_H & 0xfc00) | (mmaddr >> 0x10);
	AR_DMA_MMADDR_L = (AR_DMA_MMADDR_L & 0x1f) | mmaddr;
	AR_DMA_ARADDR_H = ((AR_DMA_ARADDR_H & 0xfc00) | ((araddr >> 0x10)) & 0x1FFF);
	AR_DMA_ARADDR_L = (AR_DMA_ARADDR_L & 0x1f) | araddr;
	AR_DMA_CNT_H    = (type << 0xf) | (AR_DMA_CNT_H & 0x7fff);
	AR_DMA_CNT_H    = (AR_DMA_CNT_H & 0xfc00) | (cntL >> 0x10);
	AR_DMA_CNT_L    = (AR_DMA_CNT_L & 0x1f) | cntL;
	OSRestoreInterrupts(level);
	//- For AUDIO_DMA_START_HI, only bits 0x03ff can be set on GCN
	//and 0x1fff on Wii
}

SfxBinEntry* findSfxBinEntry(int id) {
	SfxBinEntry *sfxBin = *pSfxBin;
	for(int i=0; i<sfxBin_numEntries; i++) {
		SfxBinEntry *entry = &sfxBin[i];
		if(entry->id == id) return entry;
	}
	exiPrintf(" *** can't find SFX ID 0x%X!\r\n", id);
	return NULL;
}
