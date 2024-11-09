#ifndef __DOL_H__
#define __DOL_H__

#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11

typedef struct {
    u32 textOffset[DOL_NUM_TEXT_SECTIONS];
    u32 dataOffset[DOL_NUM_DATA_SECTIONS];
    u32 textAddr  [DOL_NUM_TEXT_SECTIONS];
    u32 dataAddr  [DOL_NUM_DATA_SECTIONS];
    u32 textSize  [DOL_NUM_TEXT_SECTIONS];
    u32 dataSize  [DOL_NUM_DATA_SECTIONS];
    u32 bssAddr, bssSize, entryPoint;
} DolHeader;

int bootDol(const char *path) __attribute__((section(".dolLoader")));

#endif //__DOL_H__
