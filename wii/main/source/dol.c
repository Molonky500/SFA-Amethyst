#include "main.h"

void printDolHeader(DolHeader *header) {
    exiPrintf("Sect## Offset Address  EndAddr  Size\n");
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        exiPrintf("text%2d %06X %08X %08X %08X\n", i,
            header->textOffset[i], header->textAddr[i],
            header->textAddr[i] + header->textSize[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        exiPrintf("data%2d %06X %08X %08X %08X\n", i,
            header->dataOffset[i], header->dataAddr[i],
            header->dataAddr[i] + header->dataSize[i],
            header->dataSize[i]);
    }
    exiPrintf("bss    ------ %08X %08X %08X\n", header->bssAddr,
        header->bssAddr + header->bssSize,
        header->bssSize);
    exiPrintf("Entry  ------ %08X -------- --------\n",
        header->entryPoint);
}

void loadDolFromMemory(DolHeader *header) {
    printDolHeader(header);

    void *data = (void*)header;

    exiPrintf("Init bss...\n");
    memset((void*)header->bssAddr, 0, header->bssSize);
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            exiPrintf("Loading text%d...\n", i);
            //XXX this check should no longer be needed.
            if(header->textAddr[i] < 0x800031A0) {
                //do not clobber OS globals.
                //this chops off some of the game's boot code
                //but we'll take care of that.
                u32 diff = 0x800031A0 - header->textAddr[i];
                header->textAddr[i] += diff;
                header->textOffset[i] += diff;
                header->textSize[i] -= diff;
            }
            memcpy((void*)header->textAddr[i],
                data + header->textOffset[i],
                header->textSize[i]);
        }
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            exiPrintf("Loading data%d...\n", i);
            memcpy((void*)header->dataAddr[i],
                data + header->dataOffset[i],
                header->dataSize[i]);
        }
    }
}
