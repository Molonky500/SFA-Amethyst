#ifndef _AUDIOSTREAM_H_
#define _AUDIOSTREAM_H_

typedef struct {
    u16 id;
    u8 unk2;
    u8 volume;
    u16 length;
    char name[16];
} StreamsBinEntry;

#endif //_AUDIOSTREAM_H_
