#include "main.h"

void printDolHeader(DolHeader *header) {
    exiPrintf("Sect## Offset Address  EndAddr  Size\r\n");
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        exiPrintf("text%2d %6X %8X %8X %8X\r\n", i,
            header->textOffset[i], header->textAddr[i],
            header->textAddr[i] + header->textSize[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        exiPrintf("data%2d %6X %8X %8X %8X\r\n", i,
            header->dataOffset[i], header->dataAddr[i],
            header->dataAddr[i] + header->dataSize[i],
            header->dataSize[i]);
    }
    exiPrintf("bss    ------ %8X %8X %8X\r\n", header->bssAddr,
        header->bssAddr + header->bssSize,
        header->bssSize);
    exiPrintf("Entry  ------ %8X -------- --------\r\n",
        header->entryPoint);
}
