#include "main.h"
//adapted from http://fileformats.archiveteam.org/wiki/Nintendo_GameCube_/_Wii_ADP

FILE *curStreamFile = NULL;

BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
u32 offset, DVDCallback callback) {
    //exiPrintf("DVDPrepareStreamAsync(%p, 0x%X, 0x%X, %p)\n",
    //    fInfo, length, offset, callback);
    (*(u8*)0x803dc849) = 0;
    OSYieldThread();
    if(callback) callback(0, fInfo);
    return true;
}

BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
DVDCBCallback callback) {
    //exiPrintf("DVDCancelStreamAsync(%p, %p)\n",
    //    block, callback);
    OSYieldThread();
    if(callback) callback(0, block);
    return true;
}

BOOL DVDStopStreamAtEndAsync_hook(DVDCommandBlock *block,
DVDCBCallback callback) {
    //exiPrintf("DVDStopStreamAtEndAsync(%p, %p)\n",
    //    block, callback);
    OSYieldThread();
    if(callback) callback(0, block);
    return true;
}

void AISetStreamPlayState_hook(int param) {
    //exiPrintf("AISetStreamPlayState(%d)\n", param);
}

static bool pendingCallback = false;
void playStream_hook() {
	//replaces call to DVDPrepareStreamAsync in streamPlay
	int streamNo = (*(int*)0x803dc870) - 1;

	StreamsBinEntry *streamsBin = *(StreamsBinEntry**)0x803dc850;
	StreamsBinEntry *stream = &streamsBin[streamNo];
	exiPrintf("Playing stream 0x%X: %s\n", streamNo, stream->name);

	char path[256];
	sprintf(path, "%s/files/streams/%s.adp", gameRootDir, stream->name);
	curStreamFile = fopen(path, "rb");
	if(!curStreamFile) {
		exiPrintf(" *** ERROR *** failed opening: \"%s\"\n", path);
	}

	//(*(int*)0x803dc868) = streamNo + 1; //curStream
	// *(int*)0x803dc86c = 1;
	pendingCallback = true;
}

void mainLoopUpdateStream_hook() {
	void (*origFunc)() = 0x8000d55c;
	origFunc();
	dvdDoPendingCallbacks();

	if(pendingCallback) {
		//avoid race condition by not calling this right away.
		void (*callback)(int, void*) = 0x8000d5dc;
		callback(0, NULL);
		pendingCallback = false;
	}

	if(!curStreamFile) return;

	float *pStreamPos    = (float*)0x803dc858;
	float *pStreamEndPos = (float*)0x803dc84c;
	//exiPrintf("Stream pos %d / %d\n",
	//	(int)(*pStreamPos), (int)(*pStreamEndPos));

	if(*pStreamPos >= *pStreamEndPos) {
		exiPuts("Stream finished\n");
		fclose(curStreamFile);
		curStreamFile = NULL;
	}
}

#if 0
#define SAMPLES_PER_BLOCK 28

/* u8 flagsL;
u8 flagsR;
u8 flagsL2; //same as flagsL
u8 flagsR2; //same as flagsR
u8 sample[28]; // Top 4 bits are for R, bottom 4 are for L stereo.
*/

static s32 histl1;
static s32 histl2;
static s32 histr1;
static s32 histr2;

static s16 ADPDecodeSample(s32 bits, s32 q, s32& hist1,
s32& hist2) {
	s32 hist = 0;
	switch(q >> 4) {
	case 0:
		hist = 0;
		break;
	case 1:
		hist = (hist1 * 0x3c);
		break;
	case 2:
		hist = (hist1 * 0x73) - (hist2 * 0x34);
		break;
	case 3:
		hist = (hist1 * 0x62) - (hist2 * 0x37);
		break;
	}
	//hist = MathUtil::Clamp((hist + 0x20) >> 6, -0x200000, 0x1fffff);
	hist = (hist + 0x20) >> 6;
    if(hist < -0x200000) hist = -0x200000;
    if(hist >  0x1fffff) hist =  0x1fffff;

	s32 cur = (((s16)(bits << 12) >> (q & 0xf)) << 6) + hist;

	hist2 = hist1;
	hist1 = cur;

	cur >>= 6;
    if(cur < -0x8000) cur = -0x8000;
    if(cur >  0x7fff) cur =  0x7fff;

	return (s16)cur;
}

void InitFilter() {
	histl1 = 0;
	histl2 = 0;
	histr1 = 0;
	histr2 = 0;
}

void DecodeBlock(s16* pcm, const u8* adpcm) {
	for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
		pcm[i * 2]     = ADPDecodeSample(adpcm[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] & 0xf, adpcm[0], histl1, histl2);
		pcm[i * 2 + 1] = ADPDecodeSample(adpcm[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] >> 4,  adpcm[1], histr1, histr2);
	}
}
#endif
