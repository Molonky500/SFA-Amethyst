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

void loadDolFromMemory(DolHeader *header) {
    //expects the entire DOL, not just the header.
    //Used to load the DOL that the first-stage loader
    //placed at a fixed memory location.
    //printDolHeader(header);

    void *data = (void*)header;
    memset(0x80003F00, 0, 0x81800000-0x80003F00);

    exiPrintf("Init bss    .................. -> %8X..%8X [%6X]\r\n",
        header->bssAddr, header->bssAddr+header->bssSize,
        header->bssSize);
    memset((void*)header->bssAddr, 0, header->bssSize);
    DCStoreRange((void*)header->bssAddr, header->bssSize);
    ICInvalidateRange((void*)header->bssAddr, header->bssSize);

    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            exiPrintf("Copy text%2d %8X..%8X -> %8X..%8X [%6X]\r\n", i,
                data + header->textOffset[i],
                data + header->textOffset[i] + header->textSize[i],
                header->textAddr[i],
                header->textAddr[i] + header->textSize[i],
                header->textSize[i]);
            memcpy((void*)header->textAddr[i],
                data + header->textOffset[i],
                header->textSize[i]);
            DCStoreRange((void*)header->textAddr[i], header->textSize[i]);
            ICInvalidateRange((void*)header->textAddr[i], header->textSize[i]);
        }
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            exiPrintf("Copy data%2d %8X..%8X -> %8X..%8X [%6X]\r\n", i,
                data + header->dataOffset[i],
                data + header->dataOffset[i] + header->dataSize[i],
                header->dataAddr[i],
                header->dataAddr[i] + header->dataSize[i],
                header->dataSize[i]);
            memcpy((void*)header->dataAddr[i],
                data + header->dataOffset[i],
                header->dataSize[i]);
            DCStoreRange((void*)header->dataAddr[i], header->dataSize[i]);
            ICInvalidateRange((void*)header->dataAddr[i], header->dataSize[i]);
        }
    }
    exiPuts("DOL load OK\r\n");
    //L2GlobalInvalidate();
    _sync();
    ICSync();

    u32 cksum = 0;
    u8 *cdata = (u8*)header;
    u32 size = *(u32*)(DOL_LOAD_ADDR-4);
    u32 correct = *(u32*)(DOL_LOAD_ADDR-8);
    for(u32 i=0; i<size; i++) cksum += cdata[i];
    exiPrintf("DOL CKSUM = %08X [%08X]\r\n", cksum, correct);
    if(cksum != correct) {
        exiPuts("DOL CKSUM FAIL\r\n");
        STM_RebootSystem();
        while(1);
    }

    doDspPatch();
}
