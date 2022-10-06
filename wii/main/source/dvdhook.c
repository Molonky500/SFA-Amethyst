#include "main.h"

void __DVDFSInit_hook(void) {
    exiPuts("reached __DVDFSInit_hook\n");

    //since this is the first thing called back into
    //from the game, this is where we init stuff.
    //XXX move out of DVD

    initAlloc();
    exiPuts("loader2 alloc init OK\n");
    initLibc();
    exiPuts("loader2 libc init OK\n");

    initCheckThread();
    //registerThreadForDebug(OSGetCurrentThread(),  "main");
    registerThreadForDebug((OSThread*)0x803AD848, "game");
    registerThreadForDebug((OSThread*)0x803AB118, "bsod");
    registerThreadForDebug((OSThread*)0x803A54A0, "THPaudio");
    registerThreadForDebug((OSThread*)0x803A6F08, "THPdisc");
    registerThreadForDebug((OSThread*)0x803A8348, "THPvideo");

    initIpc();
    exiPuts("loader2 IPC init OK\n");

    initDvdHack();
    exiPuts("initDvdHack: OK; wait for DVD...\n");

    /*u32 *handlers = (u32*)0x80003040;
    for(int i=0; i<32; i += 4) {
        printf("IRQ[%2d]: %08X %08X %08X %08X\n", i,
            handlers[i], handlers[i+1],
            handlers[i+2], handlers[i+3]);
    }*/

    while(!dvdThreadReady) OSYieldThread();
    DVD_DPRINT("DVD READY\n");
}

bool DVDOpen_hook(const char *path, DVDFileInfo *info) {
    char newPath[1024];
    snprintf(newPath, sizeof(newPath), "%s/files/%s",
        gameRootDir, path);
    DVD_DPRINT("DVDOpen(\"%s\", %08X)... ", newPath, (u32)info);
    if(!dvdThreadReady) {
        DVD_DPRINT("wait for DVD...\n");
        while(!dvdThreadReady);
    }

    //Dolphin DVD library doesn't actually care if you close
    //a file when you're done with it.
    //So the game can re-use the same DVDFileInfo for multiple
    //files without closing them.
    //When that happens, we need to ensure we remove it from
    //the open file list, so that we don't end up returning
    //the previous file instead of the new one.
    HackDvdOpenFile *oldFile = (HackDvdOpenFile*)dvd_getFileByInfo(info);
    if(oldFile) {
        fclose(oldFile->file);
        dvd_removeFile(oldFile);
    }

    DVD_DPRINT("fopen...\n");
    FILE *file = fopen(newPath, "rb");

    int err = errno;
    DVD_DPRINT("fopen: %08X\n", (u32)file);
    if(!file) {
        if(abs(err) != ENOENT) {
            printf(" *** ERROR *** DVDOpen(\"%s\", %08X) FAILED: %d\n",
                newPath, (u32)info, err);
        }
        return false;
    }
    info->startAddr = 0;
    info->callback = NULL;
    info->cb.state = 0;
    info->cb.callback = NULL;

    fseek(file, 0, SEEK_END);
    info->length = ftell(file);
    fseek(file, 0, SEEK_SET);
    DVD_DPRINT("file=0x%08X size=%d\n", (u32)file, info->length);
    dvd_addFile(info, file, path);
    printf("DVDOpen(\"%s\", %08X) => %08X, size %08X\n", path,
        (u32)info, (u32)file, (u32)info->length);

    //OSYieldThread();
    return true;
}

int DVDRead_hook(DVDFileInfo *info, void *addr, uint size, uint offset) {
    if(!addr) {
        printf(" *** ERROR *** DVDRead with NULL address @%08X\n",
            (u32)RETURN_ADDRESS);
        return -EINVAL;
    }
    if(!areInterruptsEnabled()) {
        exiPrintf(" *** %s with interrupts idsabled @%08X\n",
            __FUNCTION__, (u32)RETURN_ADDRESS);
    }
    info->cb.state = 2; //wait

    //int pad = size & 0x1F;
    //if(pad > 0) size += 32 - pad;
    int r = sendReadToDvdThread(info, addr, size,
        offset, NULL, 0, false);
    OSYieldThread();
    return r;
}

int DVDCancelAsync_hook(DVDFileInfo *info, DVDCBCallback callback) {
    DVD_DPRINT("DVDCancelAsync(%08X, cb %08X) from %08X < %08X < %08X\n",
        (u32)info, (u32)callback,
        (u32)__builtin_extract_return_addr(__builtin_return_address(0)),
        (u32)__builtin_extract_return_addr(__builtin_return_address(1)),
        (u32)__builtin_extract_return_addr(__builtin_return_address(2)));
    info->cb.state = 10; //cancelled

    //deadlocks if called from a callback
    //while(DVD_BUSY) OSYieldThread();
    HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
    if(callback) {
        DVD_DPRINT("DVDCancelAsync callback %08X(0, %08X)\n",
            (u32)callback, (u32)info);
        callback(0, info);
    }
    if(file) {
        fclose(file->file);
        dvd_removeFile(file);
    }
    return 1;
}

s32 DVDReadPrio_hook(DVDFileInfo* info, void* addr,
s32 length, s32 offset, s32 prio) {
    if(!addr) {
        printf(" *** ERROR *** DVDReadPrio with NULL address @%08X\n",
            (u32)RETURN_ADDRESS);
        return false;
    }
    if(!areInterruptsEnabled()) {
        exiPrintf(" *** %s with interrupts idsabled @%08X\n",
            __FUNCTION__, (u32)RETURN_ADDRESS);
    }
    int r = sendReadToDvdThread(info, addr, length,
        offset, NULL, prio, false);

    OSYieldThread();
    return r;
}

bool DVDReadAsyncPrio_hook(DVDFileInfo *info, void *addr, int length,
uint offset, DVDCallback callback, int prio) {
    if(!addr) {
        printf(" *** ERROR *** DVDReadAsyncPrio with NULL address @%08X\n",
            (u32)RETURN_ADDRESS);
        if(callback) callback(-1, info);
        return false;
    }
    if(!areInterruptsEnabled()) {
        exiPrintf(" *** %s with interrupts idsabled @%08X\n",
            __FUNCTION__, (u32)RETURN_ADDRESS);
    }

    (info->cb).command = 1; //read
    (info->cb).addr = addr;
    (info->cb).length = length;
    (info->cb).offset = offset;
    (info->cb).transferredSize = 0;
    (info->cb).callback = callback;
    info->callback = callback;
    //DVDReadAsyncPrio sets cb.callback to the actual callback
    //and sets info->callback to a function that just
    //calls cb.callback

    int r = sendReadToDvdThread(info, addr, length,
        offset, callback, prio, true);
    if(r <= 0) {
        printf(" *** ERROR *** DVDReadAsyncPrio send failed: %d\n", r);
    }
    OSYieldThread();
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
