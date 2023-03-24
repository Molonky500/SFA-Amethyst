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
#define STREAM_ALARM_PERIOD_MSEC 8
#define STREAM_ALARM_PERIOD OSMillisecondsToTicks(STREAM_ALARM_PERIOD_MSEC)
#define STREAM_BUF_FRAMES 4 //frames of data to buffer
//how many msec of audio per update
#define STREAM_UPDATE_RATE ((1000.0/60.0) / STREAM_ALARM_PERIOD_MSEC)

//following shouldn't need to be changed.
//AI DMA output format:
//R/L interleaved 16-bit signed integers, big endian
#define STREAM_SAMPLE_RATE 48000 //samples per second
#define STREAM_SAMPLE_SIZE 4 //bytes per sample (s16 R, s16 L)
#define STREAM_DECODE_BUF_SIZE (STREAM_SAMPLE_RATE*STREAM_SAMPLE_SIZE*STREAM_BUF_FRAMES) //bytes

#define STREAM_BLOCK_SIZE 32 //bytes
#define STREAM_SAMPLES_PER_BLOCK 28 //per channel
//blocks per second
#define STREAM_BLOCK_RATE (STREAM_SAMPLE_RATE / STREAM_SAMPLES_PER_BLOCK)
#define STREAM_READ_BUF_SIZE (STREAM_BLOCK_RATE * STREAM_BLOCK_SIZE)
//#define STREAM_BYTES_PER_FRAME ((int)((float)STREAM_BLOCK_RATE / 60.0f) * STREAM_BLOCK_SIZE)
//no idea but this works
#define STREAM_BYTES_PER_FRAME (29 * STREAM_BLOCK_SIZE)

typedef struct {
    u8 flagsL;
    u8 flagsR;
    u8 flagsL2; //same as flagsL
    u8 flagsR2; //same as flagsR
    u8 sample[STREAM_SAMPLES_PER_BLOCK]; // Top 4 bits are for R, bottom 4 are for L stereo.
} AudioStreamBlock;

#endif //_AUDIOSTREAM_H_
