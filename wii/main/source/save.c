#include "main.h"

char saveFilePath[1024];
char profileName[64];

static void writeStub(u32 addr, s16 ret) {
    *(u32*)(addr  ) = 0x38600000 | ret; //li r3, ret
    *(u32*)(addr+4) = 0x4E800020; //blr
}

void initSaveHacks() {
    strcpy(profileName, "default");
    snprintf(saveFilePath, sizeof(saveFilePath), "%s/profiles",
        gameRootDir);

    GameWiiInterface *wii = WII_IFACE_PTR;
    wii->profileName = profileName;
    wii->saveFilePath = saveFilePath;
    if(saveFilePath[0] != 0) {
        *(u32*)0x80311910 = saveGame_initialize_hook;
        hookBranch(0x8007db74, saveGameSave_hook, 1, 0);
        hookBranch(0x8007dc0c, saveGameLoad_hook, 1, 0); //titleTryLoadSaveFiles
        //can reuse same hooks
        hookBranch(0x8007ddb0, saveGameSave_hook, 1, 0);
        hookBranch(0x8007dcb0, saveGameLoad_hook, 1, 0); //loadSaveGame
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

int mkdirRecursive(const char *path) {
    char dirName[8192];
    strncpy(dirName, path, sizeof(dirName));
    for(int i=0; dirName[i]; i++) {
        if(dirName[i] == '/') {
            dirName[i] = 0;
            int err = mkdir(dirName, S_IRWXU | S_IRWXG | S_IRWXO);
            if(err) {
                err = errno;
                exiPrintf("mkdir(%s) => %d.\r\n", dirName, err);
                if(err != EEXIST) return err;
            }
            else exiPrintf("mkdir(%s) OK\r\n", dirName);
            dirName[i] = '/';
        }
    }
    return 0;
}

//TODO: translations
FILE* openSaveFile(const char *path, const char *mode, bool notExistOk) {
    static const char *msgs[3];
    msgs[0] = "Try Again"; //lol can't do {} init here
    msgs[1] = "Continue Without Saving";
    msgs[2] = NULL;
    const char *defaultMsg = "%s (Error %d)\n"
        "Try turning off the console and checking the\n"
        "SD card to ensure it's set up correctly.\n"; //end with blank line;
    char msg[1024];

    while(1) {
        FILE *file = fopen(path, mode);
        if(file) return file;
        int err = errno;
        exiPrintf("Failed fopen(%s, %s): %d\r\n", path, mode, err);
        if(notExistOk && err == ENOENT) return NULL;
        if(err == ENOTDIR) {
            char dirName[1024];
            char *slash = strrchr(path, '/');
            memcpy(dirName, path, slash-path);
            dirName[slash-path] = 0;
            err = mkdirRecursive(path);
            if(!err) continue;
        }

        const char *detail = "Failed to open the save file.";
        switch(err) {
            case ENOTDIR:
                detail = "Unable to create the save file directory.";
                break;
            case EIO:
                detail = "I/O error accessing save file.";
                break;
            case ENOSPC:
                detail = "Not enough space on the SD card to create a save file.";
                break;
            case EROFS:
                detail = "Unable to write to the SD card.";
                break;
        }
        sprintf(msg, defaultMsg, detail, err);

        GameWiiInterface *wii = WII_IFACE_PTR;
        int result = wii->showPopupMessage(msg, msgs);
        if(result != 0) return NULL; //cancelled or selected "don't save"
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
    snprintf(path, sizeof(path), "%s/%s/slot%d.sav", saveFilePath,
        profileName, slot+1);
    FILE *file = openSaveFile(path, "wb", false);
    if(!file) return 1;
    exiPuts("Writing save file!\n");
    exiPrintf("Writing save file \"%s\"\n", data->save.saveFileName);
    //exiPrintf("Volume m=%d s=%d c=%d\n", data->global.settings.musicVolume,
    //    data->global.settings.sfxVolume,
    //    data->global.settings.cutsceneVolume);
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
    snprintf(path, sizeof(path), "%s/%s/slot%d.sav", saveFilePath,
        profileName, slot+1);
    FILE *file = openSaveFile(path, "rb", true);
    if(!file) {
        if(callback == 0x8007E748) {
            memset(save, 0, sizeof(SaveSettingsAndScores));
        }
        else memset(save, 0, sizeof(SaveGame));
        return 0;
    }
    RamSaveData buf;
    fread(&buf, sizeof(RamSaveData), 1, file);
    fclose(file);
    exiPrintf("Read %s OK\n", path);
    exiPrintf("Save file name is \"%s\"\n", buf.save.saveFileName);
    //exiPrintf("Volume m=%d s=%d c=%d cb=0x%08X\n", buf.global.settings.musicVolume,
    //    buf.global.settings.sfxVolume,
    //    buf.global.settings.cutsceneVolume, callback);
    if(callback == 0x8007E748) {
        //this is the first load, getting the global settings.
        memcpy(save, &buf.global, sizeof(SaveSettingsAndScores));
    }
    else { //this is one of the three loads at the file select menu.
        memcpy(save, &buf.save, sizeof(SaveGame));
    }

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
