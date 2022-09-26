#include "main.h"
#include "revolution/os.h"

void debugMiscSubMenu_close(const Menu *self) {
    //Close function for submenus of the debug misc menu
    curMenu = &menuDebugMisc;
    audioPlaySound(NULL, MENU_CLOSE_SOUND);
}

void menuDebugMiscHexEdit_select(const MenuItem *self, int amount) {
    if(amount) return;
    hexEditPrevMenu = curMenu;
    curMenu = &menuDebugHexEdit;
    audioPlaySound(NULL, MENU_OPEN_SOUND);
}


void menuDebugFpTest_select(const MenuItem *self, int amount) {
    if(amount) return;
    while(1) {
        volatile float a = 1.0f;
        volatile float b = 2.0f;
        volatile float c = 3.0f;
        volatile float d = 4.0f;
        volatile float e = 5.0f;
        waitNextFrame();
        float f = a+b+c+d+e;
        if(f > 15.0001f || f < 14.9999f) {
            OSReport("FPU FAIL\n");
        }
    }
}
void menuDebugCopyTest_select(const MenuItem *self, int amount) {
    if(amount) return;

    static const char *test =
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us."
        "All your base are belong to us.";
    int len = strlen(test);
    if(len > 1024) len = 1024;
    while(1) {
        volatile char msg[1024];
        memcpy(msg, test, sizeof(msg));
        waitNextFrame();
        for(int i=0; i<len; i++) {
            if(msg[i] != test[i]) {
                OSReport("COPY FAIL\n");
            }
        }
    }
}
void menuDebugHeapTest_select(const MenuItem *self, int amount) {
    if(amount) return;
    u32 size = 32768;
    while(1) {
        u32 *buf = allocTagged(size, 0xD06FECE5, "test");
        waitNextFrame();
        for(int i=0; i<size/4; i++) {
            buf[i] = i;
        }
        waitNextFrame();
        for(int i=0; i<size/4; i++) {
            if(buf[i] != i) {
                OSReport("HEAP FAIL\n");
            }
        }
        free(buf);
        size <<= 1;
        if(size >= 2097152) size = 256;
    }
}
void menuDebugFileTest_select(const MenuItem *self, int amount) {
    if(amount) return;
    DVDFileInfo file1, file2, file3;
    u8 canVal1 = 0x00, canVal2 = 0xFF;
    while(1) {
        canVal1++;
        canVal2--;
        u8 *canary1 = (u8*)allocTagged(128, 0xD00D00A0, "canary1");
        u8 *data1 = (u8*)allocTagged(128, 0xD00D00AA, "butt1");
        u8 *data2 = (u8*)allocTagged(128, 0xD00D00AB, "butt2");
        u8 *data3 = (u8*)allocTagged(128, 0xD00D00AC, "butt3");
        u8 *canary2 = (u8*)allocTagged(128, 0xD00D00A1, "canary2");
        if(!data1) OSReport("alloc1 failed\n");
        if(!data2) OSReport("alloc2 failed\n");
        if(!data3) OSReport("alloc3 failed\n");
        if(!canary1) OSReport("alloc canary1 failed\n");
        if(!canary2) OSReport("alloc canary2 failed\n");
        bool ok1 = _DVDOpen("/FONTS.bin", &file1);
        bool ok2 = _DVDOpen("/splashScreen.bin", &file2);
        bool ok3 = _DVDOpen("/GAMETEXT.bin", &file3);
        if(!ok1) OSReport("open1 failed\n");
        if(!ok2) OSReport("open2 failed\n");
        if(!ok3) OSReport("open3 failed\n");
        OSReport("sizes: %d, %d, %d canary: %08X %08X\n",
            file1.length, file2.length, file3.length,
            canary1, canary2);
        u32 offs=0, sum1=0, sum2=0, sum3=0;
        int r1, r2, r3;

        for(int i=0; i<128; i++) {
            canary1[i] = canVal1 ^ (u8)i;
            canary2[i] = canVal2 ^ (u8)i;
        }

        while(offs < file1.length || offs < file2.length
        || offs < file3.length) {
            OSReport("read %d\n", offs);
            if(offs < file1.length) r1 = _DVDRead(&file1, data1, MIN(128,offs-file1.length), offs);
            if(offs < file2.length) r2 = _DVDRead(&file2, data2, MIN(128,offs-file2.length), offs);
            if(offs < file3.length) r3 = _DVDRead(&file3, data3, MIN(128,offs-file3.length), offs);
            if(r1 < 0) OSReport("read1 failed (%d)\n");
            if(r2 < 0) OSReport("read2 failed (%d)\n");
            if(r3 < 0) OSReport("read3 failed (%d)\n");
            for(int i=0; i<MIN(128,offs-file1.length); i++) sum1 += data1[i];
            for(int i=0; i<MIN(128,offs-file2.length); i++) sum2 += data2[i];
            for(int i=0; i<MIN(128,offs-file3.length); i++) sum3 += data3[i];
            offs += 128;
        }
        if(sum1 !=  2131159) OSReport("sum1=%d\n", sum1);
        if(sum2 != 47701670) OSReport("sum2=%d\n", sum2);
        if(sum3 != 62053618) OSReport("sum3=%d\n", sum3);
        _DVDClose(&file1);
        _DVDClose(&file2);
        _DVDClose(&file3);

        for(int i=0; i<128; i++) {
            if(canary1[i] != (canVal1 ^ (u8)i)) {
                OSReport("CANARY1 FAIL at %d (%02X != %02X)\n", i,
                    canary1[i], canVal1 ^ i);
                while(1);
            }
            if(canary2[i] != (canVal2 ^ (u8)i)) {
                OSReport("CANARY2 FAIL at %d (%02X != %02X)\n", i,
                    canary2[i], canVal2 ^ i);
                while(1);
            }
        }

        OSReport("Check OK\n");
        //waitNextFrame();
    }
}
void menuDebugFileTest2_select(const MenuItem *self, int amount) {
    if(amount) return;
    static const char *files[] = {
        "/test1.bin",
        "/test2.bin",
        "/test3.bin",
        "/test4.bin",
        "/test5.bin",
        NULL
    };
    int iFile = 0;
    while(1) {
        waitNextFrame();
        u32 size;
        u32 *data = loadFileByPath(files[iFile], &size);
        if(!data) {
            OSReport("test file not found or load failed: %s\n", files[iFile]);
            iFile++;
            if(!files[iFile]) {
                iFile = 0;
                //allocTagged(128, 0xDEADDEAD, "waste");
            }
            continue;
        }
        OSReport("File %s loaded: %08X\n", files[iFile], data);
        for(int i=0; i<size/4; i++) {
            if(data[i] != i) {
                OSReport("File corrupt at %08X (%08X != %08X)\n",
                    &data[i], i, data[i]);
                while(1);
            }
        }
        OSReport("File OK\n");
        free(data);
        iFile++;
        if(!files[iFile]) {
            iFile = 0;
            //allocTagged(128, 0xDEADDEAD, "waste");
        }
    }
}
void menuDebugGprTest_select(const MenuItem *self, int amount) {
    if(amount) return;
    while(1) {
        __asm__(
            "lis 0,  0x0000\n ori 0,  0,  0x0000\n"
            "lis 3,  0x3333\n ori 3,  3,  0x3333\n"
            "lis 4,  0x4444\n ori 4,  4,  0x4444\n"
            "lis 5,  0x5555\n ori 5,  5,  0x5555\n"
            "lis 6,  0x6666\n ori 6,  6,  0x6666\n"
            "lis 7,  0x7777\n ori 7,  7,  0x7777\n"
            "lis 8,  0x8888\n ori 8,  8,  0x8888\n"
            "lis 9,  0x9999\n ori 9,  9,  0x9999\n"
            "lis 10, 0xAAAA\n ori 10, 10, 0xAAAA\n"
            "lis 11, 0xBBBB\n ori 11, 11, 0xBBBB\n"
            "lis 12, 0xCCCC\n ori 12, 12, 0xCCCC\n"
            "lis 14, 0xEEEE\n ori 14, 14, 0xEEEE\n"
            "lis 15, 0xFFFF\n ori 15, 15, 0xFFFF\n"
            "lis 16, 0x1010\n ori 16, 16, 0x1010\n"
            "lis 17, 0x1111\n ori 17, 17, 0x1111\n"
            "lis 18, 0x1212\n ori 18, 18, 0x1212\n"
            "lis 19, 0x1313\n ori 19, 19, 0x1313\n"
            "lis 20, 0x1414\n ori 20, 20, 0x1414\n"
            "lis 21, 0x1515\n ori 21, 21, 0x1515\n"
            "lis 22, 0x1616\n ori 22, 22, 0x1616\n"
            "lis 23, 0x1717\n ori 23, 23, 0x1717\n"
            "lis 24, 0x1818\n ori 24, 24, 0x1818\n"
            "lis 25, 0x1919\n ori 25, 25, 0x1919\n"
            "lis 26, 0x1A1A\n ori 26, 26, 0x1A1A\n"
            "lis 27, 0x1B1B\n ori 27, 27, 0x1B1B\n"
            "lis 28, 0x1C1C\n ori 28, 28, 0x1C1C\n"
            "lis 29, 0x1D1D\n ori 29, 29, 0x1D1D\n"
            "lis 30, 0x1E1E\n ori 30, 30, 0x1E1E\n"
            "lis 31, 0x1F1F\n ori 31, 31, 0x1F1F\n"
            "loop:\nb loop\n"
        );
    }
}

void menuDebugMiscRNG_draw(const MenuItem *self, int x, int y, bool selected) {
    char str[256];
    static const char *modes[] = {"Normal", "Zero", "One", "Half", "Max",
        "Increment", "Frame", "P4 R-Trigger"};
    sprintf(str, self->fmt, T(self->name), T(modes[rngMode]));
    menuDrawText(str, x, y, selected);
}
void menuDebugMiscRNG_select(const MenuItem *self, int amount) {
    rngMode += amount;
    if(rngMode < 0) rngMode = NUM_RNG_MODES - 1;
    if(rngMode >= NUM_RNG_MODES) rngMode = 0;
    audioPlaySound(NULL, MENU_ADJUST_SOUND);
}

void menuDebugMiscRNGTest_select(const MenuItem *self, int amount) {
    if(amount) return;

    u32 count[256], inval=0;
    memset(count, 0, sizeof(count));
    for(int i=0; i<100000000; i++) {
        u32 r = randomGetRange(0, 254);
        if(r > 255) inval++;
        else count[r]++;
    }
    char str[256], *buf;
    for(int i=0; i<256; i += 16) {
        buf = str + sprintf(str, "%3d: ", i);
        for(int j=0; j<16; j++) {
            buf = buf + sprintf(buf, "%5d ", count[i+j]);
        }
        OSReport("%s\n", str);
    }
    OSReport("Invalid: %d\n", inval);

    /*for(u32 i=0; i<0xFFFFFFFF; i++) {
        randomNumber = i;
        u32 val = randomGetRange(1, 4);
        if(val < 1 || val > 4) OSReport("%08X", i);
    }*/
}

void menuDebugMiscDie_select(const MenuItem *self, int amount) {
    if(amount) return;
    playerDie(pPlayer);
}

void menuDebugMiscCrash_select(const MenuItem *self, int amount) {
    if(amount) return;
    (*(u32*)0xBAD0) = 0xDEADDEAD;
}

Menu menuDebugMisc = {
    "Misc", 0,
    genericMenu_run, genericMenu_draw, debugSubMenu_close,
    "Edit Memory", "%s", genericMenuItem_draw,  menuDebugMiscHexEdit_select,
    "RNG", "%s: %s",     menuDebugMiscRNG_draw, menuDebugMiscRNG_select,
    "RNG Test", "%s",    genericMenuItem_draw,  menuDebugMiscRNGTest_select,
    "Kill Player", "%s", genericMenuItem_draw,  menuDebugMiscDie_select,
    "Crash Game",  "%s", genericMenuItem_draw,  menuDebugMiscCrash_select,
    "FPU Test",  "%s",   genericMenuItem_draw,  menuDebugFpTest_select,
    "Copy Test",  "%s",  genericMenuItem_draw,  menuDebugCopyTest_select,
    "Heap Test",  "%s",  genericMenuItem_draw,  menuDebugHeapTest_select,
    "File Test",  "%s",  genericMenuItem_draw,  menuDebugFileTest_select,
    "File Test2", "%s",  genericMenuItem_draw,  menuDebugFileTest2_select,
    "GPR Test",   "%s",  genericMenuItem_draw,  menuDebugGprTest_select,
    NULL,
};
