#include "main.h"

void printDolHeader(DolHeader *header) {
    printf("Sect## Offset Address  EndAddr  Size\n");
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        printf("text%2d %06X %08X %08X %08X\n", i,
            header->textOffset[i], header->textAddr[i],
            header->textAddr[i] + header->textSize[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        printf("data%2d %06X %08X %08X %08X\n", i,
            header->dataOffset[i], header->dataAddr[i],
            header->dataAddr[i] + header->dataSize[i],
            header->dataSize[i]);
    }
    printf("bss    ------ %08X %08X %08X\n", header->bssAddr,
        header->bssAddr + header->bssSize,
        header->bssSize);
    printf("Entry  ------ %08X -------- --------\n",
        header->entryPoint);
}

void loadDol(FILE *dol, DolHeader *header) {
    //printf("Init bss...\n");
    memset((void*)header->bssAddr, 0, header->bssSize);
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            //printf("Loading text%d...\n", i);
            if(header->textAddr[i] < 0x800031A0) {
                //do not clobber OS globals.
                //this chops off some of the game's boot code
                //but we'll take care of that.
                u32 diff = 0x800031A0 - header->textAddr[i];
                header->textAddr[i] += diff;
                header->textOffset[i] += diff;
                header->textSize[i] -= diff;
            }
            fseek(dol, header->textOffset[i], SEEK_SET);
            ssize_t len = header->textSize[i];
            void *dest = (void*)header->textAddr[i];
            while(len > 0) {
                size_t r = fread(dest, 1, len, dol);
                dest += r;
                len -= r;
            }
        }
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            fseek(dol, header->dataOffset[i], SEEK_SET);
            ssize_t len = header->dataSize[i];
            void *dest = (void*)header->dataAddr[i];
            while(len > 0) {
                size_t r = fread(dest, 1, len, dol);
                dest += r;
                len -= r;
            }
        }
    }
}
