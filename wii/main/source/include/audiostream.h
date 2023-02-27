#ifndef _AUDIOSTREAM_H_
#define _AUDIOSTREAM_H_

typedef struct {
    u16 id;
    u8 unk2;
    u8 volume;
    u16 length;
    char name[16];
} StreamsBinEntry;

#define STREAM_SAMPLE_RATE 32000 //samples per second
#define STREAM_SAMPLE_SIZE 1 //bytes per sample
#define STREAM_BUF_SIZE ((STREAM_SAMPLE_RATE/STREAM_SAMPLE_SIZE) * 60) //bytes
#define STREAM_DECODE_BUF_SIZE (STREAM_BUF_SIZE * 4)
#define STREAM_BLOCK_SIZE 32
#define STREAM_SAMPLES_PER_BLOCK 28
//blocks per second
#define STREAM_BLOCK_RATE (STREAM_SAMPLE_RATE / STREAM_SAMPLES_PER_BLOCK)

typedef struct {
    u8 flagsL;
    u8 flagsR;
    u8 flagsL2; //same as flagsL
    u8 flagsR2; //same as flagsR
    u8 sample[STREAM_SAMPLES_PER_BLOCK]; // Top 4 bits are for R, bottom 4 are for L stereo.
} AudioStreamBlock;

#endif //_AUDIOSTREAM_H_
