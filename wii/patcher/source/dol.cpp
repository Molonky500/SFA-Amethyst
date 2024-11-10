#include "main.h"

static u8 dolBuffer[4 * 1024 * 1024] __attribute__((section(".dolBuf")));
static u8 _loaderBuf[1 * 1024 * 1024] __attribute__((section(".dolLoader")));

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _ipcReg = (u32*)0xCD800000;

//we're successfully getting into the game code
//but it ends up at 80000308 after writing a bunch
//of bogus PI_RESET values

static void _loader(u8 *buffer) {
    DolHeader *header = (DolHeader*)buffer;
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        u32 *d = (u32*)header->textAddr[i];
        u32 *s = (u32*)&buffer[header->textOffset[i]];
        for(u32 j=0; j<header->textSize[i]; j += 4) {
            *(d++) = *(s++);
        }
    }

    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        u32 *d = (u32*)header->dataAddr[i];
        u32 *s = (u32*)&buffer[header->dataOffset[i]];
        for(u32 j=0; j<header->dataSize[i]; j += 4) {
            *(d++) = *(s++);
        }
    }

    u32 *d = (u32*)header->bssAddr;
    for(int i=0; i<header->bssSize; i += 4) *(d++) = 0;

    __asm__ __volatile__ (
        "sync\n"
        "isync\n"
        "mtctr %0\n"
        "bctr\n"
        ".int 0xDEADFACE" //marker to find end of function
    : : "r" (header->entryPoint));

    //tell the compiler we're not coming back from this one.
    __builtin_unreachable();
}

int bootDol(const char *path) {
    FILE *file = fopen(path, "rb");
    if(!file) {
        int err = errno;
        fprintf(stderr, "bootDol(%s): open failed: %d\r\n", path, err);
        return -err;
    }

    int r = fread(dolBuffer, 1, sizeof(dolBuffer), file);
    if(r < 1) {
        int err = errno;
        fprintf(stderr, "bootDol(%s): read failed: %d\r\n", path, err);
        fclose(file);
        return -err;
    }
    fclose(file);
    printf("Read %d bytes\r\n", r);
    printf("loader @%08X buffer @%08X\r\n", _loaderBuf, dolBuffer);

    printf("Sec Address  Offset Length\r\n");
    DolHeader *header = (DolHeader*)dolBuffer;
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        printf("t%2d %8X %6X %6X\r\n", i,
            header->textAddr[i],
            header->textOffset[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        printf("d%2d %8X %6X %6X\r\n", i,
            header->dataAddr[i],
            header->dataOffset[i],
            header->dataSize[i]);
    }
    printf("bss %8X ------ %6X entry %8X\r\n",
        header->bssAddr, header->bssSize, header->entryPoint);

    int size = 0;
    u32 *data = (u32*)_loader;
    while(size < 10000 && *data != 0xDEADFACE) {
        data++;
        size += 4;
    }
    if(size >= 10000) {
        fprintf(stderr, "Can't find magic marker\r\n");
        return -1;
    }

    printf("sizeof(_loader) = %zd\r\n", size);
    memcpy(_loaderBuf, (void*)_loader, size);
    DCFlushRange(_loaderBuf, size);
    ICInvalidateRange(_loaderBuf, size);

    printf("Jumping to %p\r\n", _loaderBuf);
    fatUnmount("dvdraw");
    fatUnmount("sd");
    USB_Deinitialize();
    __STM_Close();
    __IOS_ShutdownSubsystems();
    IRQ_Disable();
    LCDisable();
    //L2Disable();

    u32 RESET_BITS =
        (1 << 22) | //DSP (hangs system on boot if we do both DSP resets)
        (1 << 21) | //VI1
        (1 << 20) | //VI
        //(1 << 19) | //IOPI (reboots system)
        //(1 << 18) | //IOMEM
        (1 << 17) | //IODI
        //(1 << 16) | //IOEXI (kills debug prints)
        (1 << 15) | //IOSI
        (1 << 14) | //AI_I2S3
        (1 << 13) | //GFX
        (1 << 12) | //GFX TCPE
        (1 << 10) | //DI 2
        0;
    _ipcReg[0x194>>2] &= ~RESET_BITS; //clear = reset
    _viReg[1] = 0; //unsure, but official code does it on reset
    _viReg[0] = 0; //reset

    __asm__ __volatile__ (
        "lis   3, 0x9200\n"
        "mtctr %0\n"
        "bctr"
    : : "r" (_loaderBuf));

    return 0;
}
