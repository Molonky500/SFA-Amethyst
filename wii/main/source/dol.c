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

void loadGameDol(DolHeader *header) {
    char path[768];
    snprintf(path, sizeof(path), "%s/sys/main.dol", gameRootDir);
    FILE *file = fopen(path, "rb");
    if(!file) {
        exiPrintf(" *** ERROR *** Failed opening: %s\n", path);
        exit(1);
    }
    fread(header, 1, sizeof(DolHeader), file);

    //init the BSS section
    exiPrintf("Init bss    .................. -> %8X..%8X [%6X]\r\n",
        header->bssAddr, header->bssAddr+header->bssSize,
        header->bssSize);
    memset((void*)header->bssAddr, 0, header->bssSize);
    DCStoreRange((void*)header->bssAddr, header->bssSize);
    ICInvalidateRange((void*)header->bssAddr, header->bssSize);

    //read the text sections
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            exiPrintf("Read text%2d %8X..%8X -> %8X..%8X [%6X]\r\n", i,
                header->textOffset[i],
                header->textOffset[i] + header->textSize[i],
                header->textAddr[i],
                header->textAddr[i] + header->textSize[i],
                header->textSize[i]);
            fseek(file, header->textOffset[i], SEEK_SET);
            fread((void*)header->textAddr[i], 1, header->textSize[i], file);
            DCStoreRange((void*)header->textAddr[i], header->textSize[i]);
            ICInvalidateRange((void*)header->textAddr[i], header->textSize[i]);
        }
    }

    //read the data sections
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            exiPrintf("Read data%2d %8X..%8X -> %8X..%8X [%6X]\r\n", i,
                header->dataOffset[i],
                header->dataOffset[i] + header->dataSize[i],
                header->dataAddr[i],
                header->dataAddr[i] + header->dataSize[i],
                header->dataSize[i]);
            fseek(file, header->dataOffset[i], SEEK_SET);
            fread((void*)header->dataAddr[i], 1, header->dataSize[i], file);
            DCStoreRange((void*)header->dataAddr[i], header->dataSize[i]);
            ICInvalidateRange((void*)header->dataAddr[i], header->dataSize[i]);
        }
    }
    exiPuts("DOL load OK\r\n");
    //L2GlobalInvalidate();
    _sync();
    ICSync();
    doDspPatch();
}
