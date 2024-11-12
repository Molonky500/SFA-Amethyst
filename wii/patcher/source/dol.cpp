#include "main.h"

static u8 dolBuffer[4 * 1024 * 1024] __attribute__((section(".dolBuf")));
static u8 _loaderBuf[1 * 1024 * 1024] __attribute__((section(".dolLoader")));

//we're successfully getting into the game code
//but it eventually dies in some area that's empty, 80003498
//which should be the second instruction of memcpy
//writing BSS before or after copying doesn't matter
//apparently nothing writes there. it's not part of the DOL sections
//that's probably because our bootloader relocates a section. so
//why isn't it copying back properly?
//call is 8028f89c, in __StringWrite
//so maybe it doesn't like the environment and is printing some message
//setting the OSBootInfo gives a different crash, jumping to 0x8
//we probably need to set more things, like pDvdBi2 (but bootloader is
//supposed to be doing that)
//in fact bootloader is supposed to set OSBootInfo too so hmmmmmmmmmm
//filling in more gets us to 807e0008, also empty, and somehow
//called from 807e0000 which is also empty
//or we just get DSI from 80043960 which is in piRomFreeLevel,
//doubt that's accurate

//XXX this doesn't belong here
typedef struct DVDDiskID
{
    char      gameName[4];
    char      company[2];
    u8        diskNumber;
    u8        gameVersion;
    u8        streaming;
    u8        streamingBufSize; // 0 = default
    u8        padding[22];      // 0's are stored
} DVDDiskID;
typedef struct {
    DVDDiskID  diskId;
    u32        magic;
    u32        version;
    u32        memorySize;
    u32        consoleType;
    void*      arenaLo;         // if non NULL, overrides __ArenaLo
    void*      arenaHi;         // if non NULL, overrides FSTLocation
    void*      FSTLocation;     // Start address of "FST area"
    u32        FSTMaxLength;    // Length of "FST area"
} OSBootInfo;

//this gets copied to 0x91000000
extern "C" {
void _loader(u8 *buffer);
__asm__(
    "_loader:  \n"
    "li        4, 7+11\n" //r4 = DOL_NUM_TEXT_SECTIONS + DOL_NUM_DATA_SECTIONS
    "mr        11, 3\n" //r11 = buffer
    "addi      3, 3, -4\n" //offset buffer for lwzu

    //loop filling BSS
    ".loader_doBss:\n"
    "li        8, 0\n" //r8 = 0
    "lwz       4, 0xD8(11)\n" //r4 = bssAddr
    "addi      4, 4, -4\n" //offset for stwu
    "lwz       5, 0xDC(11)\n" //r5 = bssSize

    ".loader_bssLoop:\n"
    "stwu      8, 4(4)\n" //*(r4++) = 0
    "dcbst     0, 4\n"
    "icbi      0, 4\n"
    "addi      5, 5, -4\n" //size -= 4
    "cmpwi     5, 0\n" //if(size)
    "bne       .loader_bssLoop\n" //continue the loop

    ".loader_nextSection:\n"
    "cmpwi     4, 0\n" //if no more sections
    "beq       .loader_done\n" //go on to BSS
    "addi      4, 4, -1\n" //decrement remaining section count
    "lwzu      5, 4(3)\n" //r5 = offset[section++]
    "lwz       6, ((7+11)*4)(3)\n" //r6 = address[section]
    "lwz       7, ((7+11)*8)(3)\n" //r7 = size[section]
    "cmpwi     7, 0\n" //if(!size)
    "beq       .loader_nextSection\n" //goto next section

    //loop copying the source to destination
    "add       5, 5, 11\n" //offset => source address
    "addi      5, 5, -4\n" //offset source for lwzu
    "addi      6, 6, -4\n" //offset dest   for stwu

    ".loader_copyLoop:\n"
    "lwzu      8, 4(5)\n" //r9 = *(src++)
    "stwu      8, 4(6)\n" //*(dst++) = r9
    "dcbst     0, 6\n"
    "icbi      0, 6\n"
    "addi      7, 7, -4\n" //size -= 4
    "cmpwi     7, 0\n" //if(size)
    "bne       .loader_copyLoop\n" //continue the loop
    "b         .loader_nextSection\n" //next section

    //flush caches and go
    ".loader_done:\n"
    "lwz       3, 0xE0(11)\n" //r3 = entry point
    "sync\n"
    "isync\n"
    "mtctr     3\n"
    "bctr\n"

    //magic marker to find this function's end
    ".int 0xDEADFACE\n"
);
};

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
    u32 lol = (u32)_loader;
    u32 *data = (u32*)lol;
    void *start = (void*)data;
    while(size < 10000 && *data != 0xDEADFACE) {
        data++;
        size += 4;
    }
    if(size >= 10000) {
        fprintf(stderr, "Can't find magic marker\r\n");
        return -1;
    }

    printf("sizeof(_loader) = %zd\r\n", size);
    memcpy(_loaderBuf, start, size);
    DCFlushRange(_loaderBuf, size);
    ICInvalidateRange(_loaderBuf, size);

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

    SET_DEBUG_PORT(__LINE__);

    static const OSBootInfo myBi = {
        {'G', 'S', 'A', 'E', '0', '1', 0, 0, 1, 0}, //diskId
        0x0d15ea5e, //magic
        1, //version
        0x01800000, //memorySize
        0x10000006, //consoleType
        (void*)0x00000000, //arenaLo
        (void*)0x817EA240, //arenaHi
        (void*)0x817EA240, //FSTLocation
        0x15DA4, //FSTMaxLength
    };
    OSBootInfo *bi = (OSBootInfo*)0x80000000;
    memcpy(bi, &myBi, sizeof(OSBootInfo));

    *(u32*)0x80000040 = 0; //isDebuggerPresent
    *(u32*)0x80000044 = 0; //markedExceptions
    *(u32*)0x80000048 = 0x246DD8; //exceptionHookDest
    *(u32*)0x8000004C = 0; //exceptionHookLrSave

    /** Fill in BI2 */
    u32 *bi2 = (u32*)0x817e8240;
    *(u32*)0x800000F4 = (u32)bi2;
    memset(bi2, 0, 0x2000);
    bi2[0] = 0; //debug monitor size
    bi2[1] = 0x01800000; //pad spec
    bi2[2] = 0; //argv
    bi2[3] = 0; //argc (3 = enable TRK)
    bi2[6] = 1; //unknown
    bi2[7] = 1; //unknown
    bi2[8] = 1; //unknown

    SET_DEBUG_PORT(__LINE__);

    __asm__ __volatile__ (
        "lis   3, 0x9200\n"
        "mtctr %0\n"
        "bctr"
    : : "r" (_loaderBuf));

    return 0;
}
