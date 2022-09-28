#include "main.h"

void __DVDFSInit_hook(void) {
    exiPrintf("reached %s\n", __FUNCTION__);

    //this is a good place to init the rest of the hacks.
    //or maybe it's not? interrupts might be disabled...

    //restore Wii-exclusive IRQ handlers
    // *(u32*)0x800030ac = acrIrq;
    // *(u32*)0x800030b8 = ipcIrq;
    __UnmaskIrq(IM_PI_ACR);
    //_ipcReg[1] = 56; //IPC_WriteReg(1,56);
    //exiPrintf("%s: OK\n", __FUNCTION__);

    initDvdHack();
    exiPrintf("initDvdHack: OK; wait for DVD...\n");

    u32 *handlers = (u32*)0x80003040;
    for(int i=0; i<32; i += 4) {
        exiPrintf("IRQ[%2d]: %08X %08X %08X %08X\n", i,
            handlers[i], handlers[i+1],
            handlers[i+2], handlers[i+3]);
    }

    while(!dvdThreadReady) OSYieldThread();
    exiPrintf("DVD READY\n");
}

bool DVDOpen_hook(const char *path, DVDFileInfo *info) {
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
    exiPrintf("fopen...\n");
    FILE *file = fopen(newPath, "rb");
    int err = errno;
    exiPrintf("fopen: %08X\n", file);
    if(!file) {
        exiPrintf(" *** ERROR *** DVDOpen(%s, %08X) FAILED: %d\n",
            newPath, info, err);
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

    //OSYieldThread();
    return true;
}

int DVDRead_hook(DVDFileInfo *info, void *addr, uint size, uint offset) {
    if(!addr) {
        exiPrintf(" *** ERROR *** DVDRead with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
        return -EINVAL;
    }
    //int pad = size & 0x1F;
    //if(pad > 0) size += 32 - pad;
    int r = sendReadToDvdThread(info, addr, size,
        offset, NULL, 0, false);
    OSYieldThread();
    return r;
}

int DVDClose_hook(DVDFileInfo *info) {
#if DVD_DEBUG
    exiPrintf("DVDClose(%08X)\n", info);
#endif
    OSYieldThread();
    HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
    if(file) {
        fclose(file->file);
        dvd_removeFile(file);
    }
    return 1;
}

s32 DVDReadPrio_hook(DVDFileInfo* info, void* addr,
s32 length, s32 offset, s32 prio) {
    if(!addr) {
        exiPrintf(" *** ERROR *** DVDReadPrio with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
        return false;
    }
    int r = sendReadToDvdThread(info, addr, length,
        offset, NULL, prio, false);

    OSYieldThread();
    return r;
}

bool DVDReadAsyncPrio_hook(DVDFileInfo *info, void *addr, int length,
uint offset, DVDCallback callback, int prio) {
    if(!addr) {
        exiPrintf(" *** ERROR *** DVDReadAsyncPrio with NULL address @%08X\n",
            __builtin_extract_return_addr(__builtin_return_address(0)));
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
