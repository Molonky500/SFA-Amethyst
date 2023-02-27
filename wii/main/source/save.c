#include "main.h"

char saveFilePath[1024];
char profileName[64];

static void writeStub(u32 addr, s16 ret) {
    *(u32*)(addr  ) = 0x38600000 | ret;
    *(u32*)(addr+4) = 0x4E800020;
}

void initSaveHacks() {
    strcpy(profileName, "default");
    snprintf(saveFilePath, sizeof(saveFilePath), "%s/profiles",
        gameRootDir);

    if(saveFilePath[0] != 0) {
        hookBranch(0x800ea170, saveGame_initialize_hook, 0, 0);
        hookBranch(0x8007db74, saveGameSave_hook, 1, 0);
        hookBranch(0x8007dc0c, saveGameLoad_hook, 1, 0);
        //can reuse same hooks
        hookBranch(0x8007ddb0, saveGameSave_hook, 1, 0);
        hookBranch(0x8007dcb0, saveGameLoad_hook, 1, 0);
        hookBranch(0x802623c8, CARDMountAsync_hook, 1, 0);
        hookBranch(0x8007de48, CARDProbeEx_hook, 1, 0);
        hookBranch(0x80261a48, CARDCheckExAsync_hook, 1, 0);
        //patch some functions to just return 0
        *(u32*)0x8025eee4 = 0x4E800020; //__CARDSync
            //will return param, which is channel, which is 0.
            //0 is also CARD_RESULT_READY, so it works.
        writeStub(0x8007f83c, CARD_RESULT_READY); //card_createThisGameFile
        writeStub(0x80262490, CARD_RESULT_READY); //CARDUnmount
    }
}

BOOL saveGameSave_hook(BOOL bNoCreate, int slot, void *cbParam,
SaveGame *save, RamSaveData *data, void *callback) {
    if(saveFilePath[0] == 0) { //use GC card
        //XXX remove? or do we need to do our own card logic?
        BOOL (*origFunc)(BOOL, int, void*, SaveGame*,
        RamSaveData*, void*) = 0x8007eb44;
        return origFunc(bNoCreate, slot, cbParam, save, data, callback);
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s%d.sav", saveFilePath,
        profileName, slot+1);
    FILE *file = fopen(path, "wb");
    if(!file) {
        exiPrintf(" *** Failed opening %s to write: %d\n", path, errno);
        return 1;
    }
    exiPuts("Writing save file!\n");
    exiPrintf("Writing save file \"%s\"\n", data->save.saveFileName);
    DCFlushRange(data, sizeof(RamSaveData)); //probably unnecessary
    fwrite(data, sizeof(RamSaveData), 1, file);
    fclose(file);
    exiPrintf("Wrote %s OK\n", path);
    return 0;
}

BOOL saveGameLoad_hook(BOOL bNoCreate, int slot, void *cbParam,
SaveGame *save, RamSaveData *data, void *callback) {
    if(saveFilePath[0] == 0) { //use GC card
        BOOL (*origFunc)(BOOL, int, void*, SaveGame*,
        RamSaveData*, void*) = 0x8007eb44;
        return origFunc(bNoCreate, slot, cbParam, save, data, callback);
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s%d.sav", saveFilePath,
        profileName, slot+1);
    FILE *file = fopen(path, "rb");
    if(!file) {
        exiPrintf(" *** Failed opening %s to read: %d\n", path, errno);
        return 0;
    }
    RamSaveData buf;
    fread(&buf, sizeof(RamSaveData), 1, file);
    fclose(file);
    exiPrintf("Read %s OK\n", path);
    exiPrintf("Save file name is \"%s\"\n", buf.save.saveFileName);
    memcpy(save, &buf.save, sizeof(SaveGame));

    return 1;
}

int CARDMountAsync_hook(s32 chan, void *workArea,
CARDCallback detachCallback, CARDCallback attachCallback) {
    if(saveFilePath[0] == 0) { //use GC card
        BOOL (*origFunc)(s32, void*, CARDCallback, CARDCallback) = 0x8026220c;
        return origFunc(chan, workArea, detachCallback, attachCallback);
    }
    exiPrintf("%s\n", __FUNCTION__);
    if(attachCallback) (attachCallback)(chan, CARD_RESULT_READY);
    return CARD_RESULT_READY;
}

int CARDProbeEx_hook(int chan,s32 *memSize,s32 *sectorSize) {
    exiPrintf("%s\n", __FUNCTION__);
    if(memSize) *memSize = 64; //megabits
    if(sectorSize) *sectorSize = 8192; //must be 8K
    return CARD_RESULT_READY;
}

int CARDCheckExAsync_hook(int chan,s32 *xferBytes,CARDCallback callback) {
    exiPrintf("%s\n", __FUNCTION__);
    //XXX callback? I think it's only meant to be called if
    //the filesystem was repaired.
    return CARD_RESULT_READY;
}
