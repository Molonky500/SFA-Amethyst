#include "main.h"
//adapted from http://fileformats.archiveteam.org/wiki/Nintendo_GameCube_/_Wii_ADP

OSThread streamThread;
OSThreadQueue streamThreadQueue;
OSAlarm streamThreadAlarm;
u64 streamTime, streamPrevTime, streamEndTime;
static u8 streamThreadStack[STREAM_THREAD_STACK_SIZE];

void streamThreadAlarmCb(OSAlarm *alarm, OSContext *ctx);
void* streamThreadMain(void *param);

FILE *curStreamFile = NULL;
int16_t *streamDecodeBuf; //-> decoded samples to be played

//game variables for tracking stream position, in seconds
float *pStreamPos    = (float*)0x803dc858; //current pos
float *pStreamEndPos = (float*)0x803dc84c; //end pos
static bool streamPaused = false;

//tDelta is frames passed this tick
float *ptDelta = (float*)0x803db414;

//some ADP decoding code copied from Dolphin
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

void initStreamThread() {
	exiPuts("Init stream-thread...\n");
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

static void _finishStream() {
	audioStopSound(STREAM_REPLACE_SFX_ID);
	if(curStreamFile) {
		fclose(curStreamFile);
		curStreamFile = NULL;
		exiPrintf("Stream stopped! %d/%d\r\n",
			(int)*pStreamPos, (int)*pStreamEndPos);
	}
}

BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
u32 offset, DVDCallback callback) {
    //exiPrintf("DVDPrepareStreamAsync(%p, 0x%X, 0x%X, %p)\n",
    //    fInfo, length, offset, callback);
    (*(u8*)0x803dc849) = 0; //dvdStreamState
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
	_finishStream();
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
	streamPaused = !param;
	if(!param) _finishStream();
}

void playStream_hook() {
	//replaces call to DVDPrepareStreamAsync in streamPlay
	ADPInitFilter();
	_finishStream();

	int streamNo = (*(int*)0x803dc870) - 1;
	//streamNo = 0x1B; //test

	StreamsBinEntry *streamsBin = *(StreamsBinEntry**)0x803dc850;
	StreamsBinEntry *stream = &streamsBin[streamNo];
	exiPrintf("Playing stream 0x%X: %s, length %d\n", streamNo, stream->name,
		stream->length);

	char path[600];
	sprintf(path, "%s/files/streams/%s.adp", gameRootDir, stream->name);
	curStreamFile = fopen(path, "rb");
	if(!curStreamFile) {
		exiPrintf(" *** ERROR *** failed opening: \"%s\"\n", path);
		return;
	}
	fseek(curStreamFile, 0, SEEK_END);
	u32 totalSamples = (ftell(curStreamFile) * STREAM_SAMPLES_PER_BLOCK) / STREAM_BLOCK_SIZE;
	streamEndTime = OSMillisecondsToTicks((totalSamples*1000)/STREAM_SAMPLE_RATE);
	streamTime = 0;
	streamPrevTime = OSGetTime();
	exiPrintf("Stream file size: %d bytes = %d samples = %d frames\r\n",
		ftell(curStreamFile), totalSamples,
		totalSamples / (STREAM_SAMPLE_RATE/60));
	fseek(curStreamFile, 0, SEEK_SET);

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

MusyxSampleDirTblA* findStreamSfxEntry() {
	//find the sound effect that we want to replace,
	//and set it up appropriately.
	static const int replaceId = STREAM_REPLACE_SAMPLE_ID;
	MusyxSampleDirTblA *replaceEntry = NULL;

	//find this ID.
	for(int iFile=0; iFile<audioNumSdiFiles; iFile++) {
		AudioSdiFileListEntry *sdi = &audioSdiFiles[iFile];
		for(int iEntry=0; iEntry<sdi->nSdiChunks; iEntry++) {
			MusyxSampleDirTblA *entry = &sdi->sdiFile[iEntry];
			if(entry->soundId == replaceId) {
				replaceEntry = entry;
				iFile = 1312;
				break;
			}
		}
	}
	exiPrintf("Replacing SFX entry at 0x%08X  \r\n", replaceEntry);
	exiPrintf("ARAM offset: 0x%08X fmt 0x%08X rate %d  \r\n",
		replaceEntry->aramOffset,
		replaceEntry->formatAndNumSamples,
		replaceEntry->sampleRate);
	replaceEntry->sampleRate = STREAM_SAMPLE_RATE;
	replaceEntry->loopStartSample = 0;
	replaceEntry->formatAndNumSamples = 0x02000000 | //PCM
		(STREAM_DECODE_BUF_SIZE >> 1);
	replaceEntry->loopSampleCount =
		(replaceEntry->formatAndNumSamples & 0xFFFFFF);
	return replaceEntry;
}

void checkStreamEnd() {
	if(!streamEndTime) return;
	//no idea how you're meant to know when a stream reaches its
	//end when it has a length of zero, or why that's the case.
	//instead we need to guess based on the file length.
	u64 now = OSGetTime();
	u64 dt = 0;
	if(streamPrevTime) {
		dt = now - streamPrevTime;
		streamPrevTime = now;
	}
	streamTime += dt * (physicsTimeScale/60.0f);

	if(streamTime >= streamEndTime) {
		//clear the buffer for when the game decides it would
		//rather not stop playing it when told
		memset(streamDecodeBuf, 0, STREAM_DECODE_BUF_SIZE);
		streamEndTime = 0;
		audioStopSound(STREAM_REPLACE_SFX_ID); //futily try again
		exiPuts("Cleared stream buffer\r\n");
	}
}

void* streamThreadMain(void *param) {
	exiPrintf("Stream thread online; update rate %d\n",
		(int)STREAM_UPDATE_RATE);
	u32 *aramUsed = 0x803de384;
	void *aramBase = 0x90000000;

	while(!curStreamFile) OSSleepThread(&streamThreadQueue);
	MusyxSampleDirTblA *sfxEntry = findStreamSfxEntry();
	exiPrintf("Alloc stream buffers (%d bytes) at 0x%X\n",
		STREAM_DECODE_BUF_SIZE, *aramUsed);
	streamDecodeBuf = aramBase + *aramUsed;
	*aramUsed += STREAM_DECODE_BUF_SIZE;
	exiPrintf("Free ARAM: %dK\n", *(u32*)0x800000D0 - *aramUsed);
	sfxEntry->aramOffset = ((u32)streamDecodeBuf) & 0x7FFFFFFF;
	u32 nSamples = sfxEntry->formatAndNumSamples & 0xFFFFFF;

	exiPrintf("Decoding stream to 0x%08X, %d samples  \r\n",
		streamDecodeBuf, nSamples);

	//HACK to fix volume resetting to 0 on load
	//void (*audioSetVolumes)(uint volume,uint scale,int bMusic,
	//	int bSfx,int bCutscenes) = 0x80009a28;
	//audioSetVolumes(100, 1000, 1, 1, 1); //no idea what scale is

	bool restart = true;
	u32 iSample = 0;
	static u64 prevTime = 0; //must be a constant
	if(!prevTime) prevTime = OSGetTime(); //ugh

	while(1) {
		if(!curStreamFile) restart = true;
		do { //always sleep at least once per iteration
			OSSleepThread(&streamThreadQueue);
			checkStreamEnd();
		} while(!curStreamFile);
		if(restart) {
			exiPrintf("Starting stream\r\n");
			audioPlaySound(NULL, STREAM_REPLACE_SFX_ID);
			restart = false;
			iSample = 0;
			*pStreamPos = 0;
		}

		int samplePos = (*pStreamPos    * (float)STREAM_SAMPLE_RATE);
		int sampleEnd = (*pStreamEndPos * (float)STREAM_SAMPLE_RATE);
		int remain = sampleEnd - samplePos;
		//sampleEnd isn't reliable because some streams inexplicably
		//have a length of 0, which defaults to 9 billion.
		//exiPrintf("Stream pos %d / %d\n",
		//	samplePos, sampleEnd);

		s32 iStartBlock = (*pStreamPos) * STREAM_BLOCK_RATE;
		iSample = (iStartBlock * STREAM_SAMPLES_PER_BLOCK) % nSamples;

		if(streamPaused) {
			memset(&streamDecodeBuf[iSample], 0, STREAM_SAMPLES_PER_BLOCK * STREAM_SAMPLE_SIZE);
			DCFlushRange(&streamDecodeBuf[iSample],
				STREAM_SAMPLES_PER_BLOCK * STREAM_SAMPLE_SIZE);
			continue;
		}

		//read a few blocks ahead.
		u8 block[STREAM_BLOCK_SIZE];
		fseek(curStreamFile, iStartBlock * STREAM_BLOCK_SIZE, SEEK_SET);
		for(int i=0; i<STREAM_BUF_FRAMES * STREAM_SAMPLE_RATE;
		i += STREAM_SAMPLES_PER_BLOCK / 2) {
			SET_DISC_LED(1);
			int r = fread(block, 1, STREAM_BLOCK_SIZE, curStreamFile);
			SET_DISC_LED(0);
			if(!r) { //reached end
				exiPuts("Stream decode finished\n");
				_finishStream();
				break;
			}
			if(r < STREAM_BLOCK_SIZE) {
				exiPrintf("Stream read %d but only got %d\r\n", STREAM_BLOCK_SIZE, r);
			}
			//ADPDecodeBlockMono(&streamDecodeBuf[iSample], block);
			for(int ibs = 0; ibs < STREAM_SAMPLES_PER_BLOCK; ibs++) {
				s32 a = ADPDecodeSample(block[ibs + (STREAM_BLOCK_SIZE - STREAM_SAMPLES_PER_BLOCK)] & 0xf, block[0], &histl1, &histl2);
				s32 b = ADPDecodeSample(block[ibs + (STREAM_BLOCK_SIZE - STREAM_SAMPLES_PER_BLOCK)] >> 4,  block[1], &histr1, &histr2);
				streamDecodeBuf[iSample] = (a+b)/2;
				iSample++;
				if(iSample >= nSamples) iSample = 0;
				remain--;
			}
			DCFlushRange(&streamDecodeBuf[iSample],
				STREAM_SAMPLES_PER_BLOCK * STREAM_SAMPLE_SIZE);
		}

		if(remain <= 0) {
			exiPuts("Reached end of stream\n");
			_finishStream();
		}
		u64 now = OSGetTime();
		u64 dt  = OSTicksToMilliseconds(now - prevTime);
		prevTime = now;
		*pStreamPos += ((float)dt) * 0.001f * (physicsTimeScale/60.0f);
		//OSYieldThread();
	}
}
