#include "main.h"

void __DVDFSInit_hook(void) {
    exiPrintf("reached %s\n", __FUNCTION__);

    //this is a good place to init the rest of the hacks.
    //or maybe it's not? interrupts might be disabled...
    switchToOgc();
    __IPC_Reinitialize();
    exiPrintf("IPC reinit OK\n");
    /*__IOS_InitializeSubsystems();
    exiPrintf("IOS ver %08X pref %08X\n",
        IOS_GetVersion(),
        IOS_GetPreferredVersion());
    IOS_ReloadIOS(IOS_GetPreferredVersion());
    exiPrintf("IOS reload OK\n");*/
    initThreads();
    exiPrintf("initThreads: OK\n");
    //exiPrintf("%s: OK\n", __FUNCTION__);
    switchToGame();
}

bool DVDOpen_hook(const char *path, DVDFileInfo *info) {
    switchToOgc();

    static bool startedThread = false;
    if(!startedThread) {
        initDvdHack();
        exiPrintf("initDvdHack: OK\n");
        switchToGame();
        while(!dvdThreadReady) OSYieldThread();
        switchToOgc();
    }

    char newPath[4096];
    snprintf(newPath, sizeof(newPath), "%s/files/%s",
        gameRootDir, path);
//#if DVD_DEBUG
    exiPrintf("DVDOpen(%s, %08X)... ", newPath, info);
//#endif
    if(!dvdThreadReady) {
        exiPuts("wait for DVD...\n");
        while(!dvdThreadReady);
    }
    FILE *file = fopen(newPath, "rb");
    if(!file) {
        int err = errno;
        exiPrintf(" *** ERROR *** DVDOpen(%s, %08X) FAILED: %d\n",
            newPath, info, err);
        switchToGame();
        return false;
    }
    dvd_addFile(info, file);
    info->startAddr = 0;
    info->callback = NULL;
    info->cb.state = 0;

    fseek(file, 0, SEEK_END);
    info->length = ftell(file);
    fseek(file, 0, SEEK_SET);

//#if DVD_DEBUG
    exiPrintf("file=0x%08X size=%d\n", file, info->length);
//#endif

    OSYieldThread();
    switchToGame();
    return true;
}

int DVDRead_hook(DVDFileInfo *info, void *addr, uint size, uint offset) {
    switchToOgc();

    if(!addr) {
        exiPrintf(" *** ERROR *** DVDRead with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
        switchToGame();
        return -EINVAL;
    }
    //int pad = size & 0x1F;
    //if(pad > 0) size += 32 - pad;
    int r = sendReadToDvdThread(info, addr, size,
        offset, NULL, 0, false);
    OSYieldThread();
    switchToGame();
    return r;
}

int DVDClose_hook(DVDFileInfo *info) {
    switchToOgc();
#if DVD_DEBUG
    exiPrintf("DVDClose(%08X)\n", info);
#endif
    OSYieldThread();
    HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
    if(file) {
        fclose(file->file);
        dvd_removeFile(file);
    }
    switchToGame();
    return 1;
}

s32 DVDReadPrio_hook(DVDFileInfo* info, void* addr,
s32 length, s32 offset, s32 prio) {
    switchToOgc();

    if(!addr) {
        exiPrintf(" *** ERROR *** DVDReadPrio with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
        switchToGame();
        return false;
    }
    int r = sendReadToDvdThread(info, addr, length,
        offset, NULL, prio, false);

    OSYieldThread();
    switchToGame();
    return r;
}

bool DVDReadAsyncPrio_hook(DVDFileInfo *info, void *addr, int length,
uint offset, DVDCallback callback, int prio) {
    switchToOgc();

    if(!addr) {
        exiPrintf(" *** ERROR *** DVDReadAsyncPrio with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
        switchToGame();
        if(callback) callback(-1, info);
        return false;
    }

    (info->cb).command = 1;
    (info->cb).addr = addr;
    (info->cb).length = length;
    (info->cb).offset = offset;
    (info->cb).transferredSize = 0;
    (info->cb).callback = callback;

    int r = sendReadToDvdThread(info, addr, length,
        offset, callback, prio, true);
    if(r <= 0) {
        exiPrintf(" *** ERROR *** DVDReadAsyncPrio send failed: %d\n", r);
    }
    OSYieldThread();
    switchToGame();
    return (r > 0);
}

BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
u32 offset, DVDCallback callback) {
    callback(0, fInfo);
    OSYieldThread();
    return true;
}

BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
DVDCBCallback callback) {
    callback(0, block);
    OSYieldThread();
    return true;
}
