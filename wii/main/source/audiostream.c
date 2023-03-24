#include "main.h"
//adapted from http://fileformats.archiveteam.org/wiki/Nintendo_GameCube_/_Wii_ADP

OSThread streamThread;
OSThreadQueue streamThreadQueue;
OSAlarm streamThreadAlarm;
u64 streamStartTime;
static u8 streamThreadStack[STREAM_THREAD_STACK_SIZE];

void streamThreadAlarmCb(OSAlarm *alarm, OSContext *ctx);
void* streamThreadMain(void *param);

FILE *curStreamFile = NULL;
//uint8_t *streamBuf = NULL;
//int16_t *streamDecodeBuf = NULL;
//__attribute__ ((aligned (32))) uint8_t streamBuf[STREAM_READ_BUF_SIZE];
__attribute__ ((aligned (32))) int16_t streamDecodeBuf1[STREAM_DECODE_BUF_SIZE / 2];
__attribute__ ((aligned (32))) int16_t streamDecodeBuf2[STREAM_DECODE_BUF_SIZE / 2];
int16_t *streamDecodeBuf;

void initStreamThread() {
	exiPuts("Init stream-thread...\n");
	streamDecodeBuf = streamDecodeBuf1;
	OSCreateAlarm(&streamThreadAlarm);
	OSInitThreadQueue(&streamThreadQueue);
    OSCreateThread(&streamThread, streamThreadMain,
        NULL, streamThreadStack+STREAM_THREAD_STACK_SIZE,
        STREAM_THREAD_STACK_SIZE, STREAM_THREAD_PRIO, 0);
	registerThreadForDebug(&streamThread, "stream");
	OSSetAlarm(&streamThreadAlarm, STREAM_ALARM_PERIOD,
        streamThreadAlarmCb);
    OSResumeThread(&streamThread);
}

BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
u32 offset, DVDCallback callback) {
    //exiPrintf("DVDPrepareStreamAsync(%p, 0x%X, 0x%X, %p)\n",
    //    fInfo, length, offset, callback);
    (*(u8*)0x803dc849) = 0;
    OSYieldThread();
    if(callback) dvdAddPendingStreamCallback(callback, fInfo);
    return true;
}

BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
DVDCBCallback callback) {
    exiPrintf("DVDCancelStreamAsync(%p, %p)\n",
        block, callback);
    OSYieldThread();
    if(callback) callback(0, block);
	if(curStreamFile) fclose(curStreamFile);
	curStreamFile = NULL;
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
    exiPrintf("AISetStreamPlayState(%d)\n", param);
}

void playStream_hook() {
	//replaces call to DVDPrepareStreamAsync in streamPlay
	/*if(!streamBuf) {
		streamBuf = malloc(STREAM_READ_BUF_SIZE);
		streamDecodeBuf = malloc(STREAM_DECODE_BUF_SIZE);
	}
	if(!(streamBuf && streamBuf)) {
		exiPrintf(" *** ERROR *** Can't allocate %d bytes for stream buffer\n",
			STREAM_READ_BUF_SIZE + STREAM_DECODE_BUF_SIZE);
		if(streamBuf) free(streamBuf);
		if(streamDecodeBuf) free(streamDecodeBuf);
		streamBuf = NULL;
		streamDecodeBuf = NULL;
	}*/
	ADPInitFilter();

	int streamNo = (*(int*)0x803dc870) - 1;
	//streamNo = 0x1B; //test

	StreamsBinEntry *streamsBin = *(StreamsBinEntry**)0x803dc850;
	StreamsBinEntry *stream = &streamsBin[streamNo];
	exiPrintf("Playing stream 0x%X: %s\n", streamNo, stream->name);

	char path[600];
	sprintf(path, "%s/files/streams/%s.adp", gameRootDir, stream->name);
	curStreamFile = fopen(path, "rb");
	if(!curStreamFile) {
		exiPrintf(" *** ERROR *** failed opening: \"%s\"\n", path);
	}
	streamStartTime = OSGetTime();

	//(*(int*)0x803dc868) = streamNo + 1; //curStream
	// *(int*)0x803dc86c = 1;
	dvdAddPendingStreamCallback(0x8000d5dc, NULL);
}

void mainLoopUpdateStream_hook() {
	//just updates streamPos
	//void (*origFunc)() = 0x8000d55c;
	//origFunc();
	dvdDoPendingStreamCallbacks();
}

void streamThreadAlarmCb(OSAlarm *alarm, OSContext *ctx) {
    //set alarm again because we don't have OSSetPeriodicAlarm
    OSSetAlarm(&streamThreadAlarm, STREAM_ALARM_PERIOD,
        streamThreadAlarmCb);
    OSWakeupThread(&streamThreadQueue);
}

static void _finishStream() {
	fclose(curStreamFile);
	curStreamFile = NULL;
}

void* streamThreadMain(void *param) {
	exiPrintf("Stream thread online; update rate %d\n",
		(int)STREAM_UPDATE_RATE);
	while(1) {
		do { //always sleep at least once per iteration
			OSSleepThread(&streamThreadQueue);
		} while(!curStreamFile);
		float *pStreamPos    = (float*)0x803dc858;
		float *pStreamEndPos = (float*)0x803dc84c;

		u64 tDiff = OSGetTime() - streamStartTime;
		u32 streamTime = OSTicksToMilliseconds(tDiff);
		*pStreamPos = streamTime / 1000.0f;

		int samplePos = (*pStreamPos    * (float)STREAM_SAMPLE_RATE);
		int sampleEnd = (*pStreamEndPos * (float)STREAM_SAMPLE_RATE);
		int remain = sampleEnd - samplePos;
		//sampleEnd isn't reliable for some reason; likes to read 0x7FFFFFFF
		//exiPrintf("Stream pos %d / %d\n",
		//	samplePos, sampleEnd);

		u32 iStartBlock = ((float)streamTime * STREAM_BLOCK_RATE) / 1000.0f;
		//XXX multiply by time scale to allow for fast forward
		//exiPrintf("Stream block %d\n", iStartBlock);

		//this works but is staticky.
		//we aren't really doing double-buffer correctly.
		//should be swapping buffers when the DMA is finished,
		//and appending to the buffer rather than always writing
		//at the beginning.
		//might also end up having to resample because right now
		//we change the DSP sample rate which will likely screw up
		//all the sound effects.
		//right now they aren't playing at all during/after a stream...
		//maybe we need to splice into the game's sound effect handler
		//and inject the decoded stream as a sound effect?

		if(remain > 0) {
			int size = STREAM_BYTES_PER_FRAME;
			if(size > remain) size = remain;

			fseek(curStreamFile, iStartBlock * STREAM_BLOCK_SIZE, SEEK_SET);
			remain -= size;

			uint32_t iByte   = 0;
			uint32_t iSample = 0;
			u8 block[STREAM_BLOCK_SIZE];
			while(iByte < size) {
				int r = fread(block, 1, STREAM_BLOCK_SIZE, curStreamFile);
				if(!r) { //reached end
					exiPuts("Stream finished\n");
					_finishStream();
					break;
				}
				ADPDecodeBlock(&streamDecodeBuf[iSample], block);
				iSample += STREAM_SAMPLES_PER_BLOCK * 2;
				iByte   += STREAM_BLOCK_SIZE;
			}

			DCFlushRange(streamDecodeBuf, iSample);
			u32 addr = MEM_VIRTUAL_TO_PHYSICAL(streamDecodeBuf);
			//set DSP sample rate to 32KHz
			_aiReg[0] = (_aiReg[0] & ~(1<<6)) ;//| (1<<6);
			AI_DMA_START_HI = addr >> 16;
			AI_DMA_START_LO = addr & 0xFFFF;
			AI_DMA_LENGTH   = ((iSample) >> 5) | 0x8000;

			if(streamDecodeBuf == streamDecodeBuf1) {
				streamDecodeBuf = streamDecodeBuf2;
			}
			else streamDecodeBuf = streamDecodeBuf1;
		}

		if(remain <= 0) {
			exiPuts("Reached end of stream\n");
			_finishStream();
		}
		OSYieldThread();
	}
}

static s32 histl1 = 0;
static s32 histl2 = 0;
static s32 histr1 = 0;
static s32 histr2 = 0;

static s16 ADPDecodeSample(s32 bits, s32 q, s32 *hist1,
s32 *hist2) {
	s32 hist = 0;
	switch(q >> 4) {
	case 0:
		hist = 0;
		break;
	case 1:
		hist = (*hist1 * 0x3c);
		break;
	case 2:
		hist = (*hist1 * 0x73) - (*hist2 * 0x34);
		break;
	case 3:
		hist = (*hist1 * 0x62) - (*hist2 * 0x37);
		break;
	}
	hist = (hist + 0x20) >> 6;
    if(hist < -0x200000) hist = -0x200000;
    if(hist >  0x1fffff) hist =  0x1fffff;

	s32 cur = (((s16)(bits << 12) >> (q & 0xf)) << 6) + hist;

	*hist2 = *hist1;
	*hist1 = cur;

	cur >>= 6;
    if(cur < -0x8000) cur = -0x8000;
    if(cur >  0x7fff) cur =  0x7fff;
	return (s16)cur;
}

void ADPInitFilter() {
	histl1 = 0;
	histl2 = 0;
	histr1 = 0;
	histr2 = 0;
}

void ADPDecodeBlock(s16* pcm, const u8* adpcm) {
	for (int i = 0; i < STREAM_SAMPLES_PER_BLOCK; i++) {
		pcm[i * 2]     = ADPDecodeSample(adpcm[i + (STREAM_BLOCK_SIZE - STREAM_SAMPLES_PER_BLOCK)] & 0xf, adpcm[0], &histl1, &histl2);
		pcm[i * 2 + 1] = ADPDecodeSample(adpcm[i + (STREAM_BLOCK_SIZE - STREAM_SAMPLES_PER_BLOCK)] >> 4,  adpcm[1], &histr1, &histr2);
	}
}
