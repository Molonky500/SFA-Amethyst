#include "main.h"

static u8 dolBuffer[4 * 1024 * 1024] __attribute__((section(".dolBuf")));

int bootDol(const char *path) {
    FILE *file = fopen(path, "rb");
    if(!file) return -errno;

    int r = fread(dolBuffer, 1, sizeof(dolBuffer), file);
    if(r < 1) return r;

    DolHeader *header = (DolHeader*)dolBuffer;
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i]) {
            memcpy((void*)header->textAddr[i],
                &dolBuffer[header->textOffset[i]],
                header->textSize[i]);
            DCFlushRange((void*)header->textAddr[i],
                header->textSize[i]);
        }
    }

    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i]) {
            memcpy((void*)header->dataAddr[i],
                &dolBuffer[header->dataOffset[i]],
                header->dataSize[i]);
            DCFlushRange((void*)header->dataAddr[i],
                header->dataSize[i]);
        }
    }

    memset((void*)header->bssAddr, 0, header->bssSize);
    DCFlushRange((void*)header->bssAddr, header->bssSize);

    typedef void (*DolEntry)(void);
    DolEntry ent = (DolEntry)header->entryPoint;
    ent();
    return 0;
}
