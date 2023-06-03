#ifndef _AUDIOSTREAM_H_
#define _AUDIOSTREAM_H_

typedef struct {
    u16 id;
    u8 unk2;
    u8 volume;
    u16 length;
    char name[16];
} StreamsBinEntry;

#define STREAM_THREAD_PRIO 2 //31=lowest 0=highest
#define STREAM_THREAD_STACK_SIZE 65536
#define STREAM_ALARM_PERIOD_MSEC 5
#define STREAM_ALARM_PERIOD OSMillisecondsToTicks(STREAM_ALARM_PERIOD_MSEC)
#define STREAM_BUF_FRAMES 1 //frames of data to buffer.
	//increasing might give smoother sound but harms FPS.
	//we have enough ARAM for up to 6, possibly 7 (untested).
	//beyond that will crash.
	//(the buffer doesn't need to be in ARAM, but that's where
	//I've put it...)
//how many msec of audio per update
#define STREAM_UPDATE_RATE ((1000.0/60.0) / STREAM_ALARM_PERIOD_MSEC)

//following shouldn't need to be changed.
//#define STREAM_REPLACE_SAMPLE_ID 0x25C //unused "decoy" voice clip
#define STREAM_REPLACE_SAMPLE_ID 0x4CF //unused "decoy" voice clip
#define STREAM_REPLACE_SFX_ID 0x3F7
//AI DMA output format:
//R/L interleaved 16-bit signed integers, big endian
#define STREAM_SAMPLE_RATE 48000 //samples per second
#define STREAM_SAMPLE_SIZE 4 //bytes per sample (s16 R, s16 L)
#define STREAM_DECODE_BUF_SIZE \
	(STREAM_SAMPLE_RATE*STREAM_SAMPLE_SIZE*STREAM_BUF_FRAMES) //bytes

#define STREAM_BLOCK_SIZE 32 //bytes
#define STREAM_SAMPLES_PER_BLOCK 28 //per channel
//blocks per second
#define STREAM_BLOCK_RATE ((STREAM_SAMPLE_RATE / STREAM_SAMPLES_PER_BLOCK))
#define STREAM_READ_BUF_SIZE (STREAM_BLOCK_RATE * STREAM_BLOCK_SIZE)
//#define STREAM_BYTES_PER_FRAME ((int)((float)STREAM_BLOCK_RATE / 60.0f) * STREAM_BLOCK_SIZE)
//no idea but this works
#define STREAM_BYTES_PER_FRAME (29 * STREAM_BLOCK_SIZE)
//#define STREAM_BYTES_PER_FRAME (128*STREAM_BLOCK_SIZE)

typedef struct {
    u8 flagsL;
    u8 flagsR;
    u8 flagsL2; //same as flagsL
    u8 flagsR2; //same as flagsR
    u8 sample[STREAM_SAMPLES_PER_BLOCK]; // Top 4 bits are for R, bottom 4 are for L stereo.
} AudioStreamBlock;

//XXX lots of duplication from amethyst here.

typedef struct PACKED SfxBinEntry {
	s16 id;         //0x00 SoundId
	u8  baseVolume; //0x02
	u8  volumeRand; //0x03 volume = rand(baseVolume - volumeRand, baseVolume + volumeRand)
	u8  basePan;    //0x04 127 = center
	u8  panRand;    //0x05 never used, but works same as volumeRand
	u16 unk06;      //0x06
	u16 range;      //0x08 how far from source object to silence
	u16 fxIds[6];   //0x0A actual sound to play (not same as SoundId)
	u8  fxChance[6];//0x16 chance to pick each sound
	u16 randMax;    //0x1C sum of fxChance
	u8  unk1E;      //0x1E maybe queue slot? high 4bits are idx into sfxTable_803db248
	u8  numIdxs : 4;//0x1F num items in randVals
	u8  prevIdx : 4;//0x1F previously played idx, to avoid playing same random idx twice in a row.
} SfxBinEntry;

#define pSfxBin ((SfxBinEntry**)0x803dc830)
#define sfxBin_numEntries (*(u32*)0x803dc834)

typedef struct {
    union {
        struct {
            u16 soundId;
            u16 padding02;
        };
        u32 end; //0xFFFFFFFF indicates end of table.
    };
	u32 offset;
    union {
	    u32 aramOffset;
        void *sampleData; //replaced with aramOffset when uploaded
    };
	u8  baseNote;
	u8  padding0D;
	u16 sampleRate;
	u32 formatAndNumSamples; //high byte is format
	u32 loopStartSample;
	u32 loopSampleCount;
	u32 tableBoffset;
} MusyxSampleDirTblA;

typedef struct {
	MusyxSampleDirTblA *sdiFile;
	void *samFile;
	s16 nSdiChunks;
	s16 padding;
} AudioSdiFileListEntry;

#define MAX_SDI_FILES 128
//extern AudioSdiFileListEntry audioSdiFiles[MAX_SDI_FILES]; //803bfc78
//extern s16 audioNumSdiFiles; //803de288

#define audioSdiFiles ((AudioSdiFileListEntry*)0x803bfc78)
#define audioNumSdiFiles (*(s16*)0x803de288)
#define aramUsedSize (*(u32*)0x803de384)

#endif //_AUDIOSTREAM_H_
