#include "main.h"

DvdPendingReadCb pendingReadCb[DVD_MAX_READ_CALLBACKS];
int nPendingReadCbs = 0;
OSMutex dvdPendingReadCbMutex;

DvdPendingStreamCb pendingStreamCb[DVD_MAX_STREAM_CALLBACKS];
int nPendingStreamCbs = 0;

DvdPendingCancelCb pendingCancelCb[DVD_MAX_CANCEL_CALLBACKS];
int nPendingCancelCbs = 0;

void dvdDumpPendingReadCallbacks() {
    if(!OSTryLockMutex(&dvdPendingReadCbMutex)) {
        exiPuts("Can't lock dvdPendingReadCbMutex\n");
        return;
    }
    exiPrintf("Pending read callbacks: %d\n", nPendingReadCbs);
    for(int i=0; i<nPendingReadCbs; i++) {
        exiPrintf("  %08x(%d, %08x): %08x: %s\n",
            pendingReadCb[i].cb,
            pendingReadCb[i].result,
            pendingReadCb[i].file->info,
            pendingReadCb[i].file,
            dvdGetFilePath(pendingReadCb[i].file));
    }
    OSUnlockMutex(&dvdPendingReadCbMutex);
}

void dvdDoPendingReadCallbacks() {
    if(!OSTryLockMutex(&dvdPendingReadCbMutex)) return;
    while(nPendingReadCbs) {
        DVD_DPRINT("%d read callbacks\n", nPendingReadCbs);
        DVDCallback cb = pendingReadCb[0].cb;
        HackDvdOpenFile *file = pendingReadCb[0].file;
        int result = pendingReadCb[0].result;
        DVD_DPRINT("DVD read callback for file %08X: %08X(%d, %08X): %s\r\n",
            file, cb, result, file->info, dvdGetFilePath(file));
        cb(result, file->info);
        DVD_DPRINT("DVD read callback for file %08X done\r\n", file);

        memcpy(&pendingReadCb[0], &pendingReadCb[1],
            sizeof(DvdPendingReadCb) * (DVD_MAX_READ_CALLBACKS - 1));
        nPendingReadCbs--;
    }
    OSUnlockMutex(&dvdPendingReadCbMutex);
}

void dvdAddPendingReadCallback(DVDCallback callback,
HackDvdOpenFile *file, int result) {
    OSLockMutex(&dvdPendingReadCbMutex);

    //is this already enqueued?
    for(int i=0; i<nPendingReadCbs; i++) {
        DvdPendingReadCb *cb = &pendingReadCb[i];
        if(cb->cb == callback && cb->file == file) {
            cb->result += result;
            return;
        }
    }

    if(nPendingReadCbs >= DVD_MAX_READ_CALLBACKS) {
        exiPuts(" *** ERROR *** too many DVD read callbacks\n");
    }
    else {
        DVD_DPRINT("Enqueue DVD read cb %d: %p(%p, %d): %s\n",
            nPendingReadCbs, callback, file, result,
            dvdGetFilePath(file));
        pendingReadCb[nPendingReadCbs].cb = callback;
        pendingReadCb[nPendingReadCbs].file = file;
        pendingReadCb[nPendingReadCbs].result = result;
        nPendingReadCbs++;
    }
    OSUnlockMutex(&dvdPendingReadCbMutex);
}

void dvdAddPendingStreamCallback(void *cb, void *param) {
	if(nPendingStreamCbs >= DVD_MAX_STREAM_CALLBACKS) {
		exiPuts(" *** ERROR *** too many DVD stream callbacks\n");
	}
	else {
		pendingStreamCb[nPendingStreamCbs].cb = cb;
		pendingStreamCb[nPendingStreamCbs].param = param;
		nPendingStreamCbs++;
	}
}

void dvdDumpPendingStreamCallbacks() {
    exiPrintf("Pending stream callbacks: %d\n", nPendingStreamCbs);
    for(int i=0; i<nPendingStreamCbs; i++) {
        exiPrintf("  %08x(0, %08x): %s\n",
            pendingStreamCb[i].cb, pendingStreamCb[i].param);
    }
}

void dvdDoPendingStreamCallbacks() {
	while(nPendingStreamCbs) {
		void (*cb)(int, void*) = pendingStreamCb[0].cb;
		void *param = pendingStreamCb[0].param;
		DVD_DPRINT("Stream CB %p(0, %p)\n", cb, param);
		cb(0, param);
		memcpy(&pendingStreamCb[0], &pendingStreamCb[1],
			sizeof(DvdPendingStreamCb) * (DVD_MAX_STREAM_CALLBACKS - 1));
        nPendingStreamCbs--;
	}
}

void dvdAddPendingCancelCallback(DVDCBCallback *callback, DVDFileInfo *info) {
    //is this callback already registered?
    for(int i=0; i<nPendingCancelCbs; i++) {
        if(pendingCancelCb[i].cb == callback
        && pendingCancelCb[i].info == info) {
            return;
        }
    }
    if(nPendingCancelCbs >= DVD_MAX_CANCEL_CALLBACKS) {
        exiPuts(" *** ERROR *** too many pending DVD cancel callbacks\n");
    }
    else {
        pendingCancelCb[nPendingCancelCbs].cb = callback;
        pendingCancelCb[nPendingCancelCbs].info = info;
        nPendingCancelCbs++;

        DVD_DPRINT("Now %d cancel callbacks:\n", nPendingCancelCbs);
        for(int i=0; i<nPendingCancelCbs; i++) {
            DvdPendingCancelCb *cb = &pendingCancelCb[i];
            HackDvdOpenFile *file = dvd_getFileByInfo(cb->info);
            DVD_DPRINT("  %p(0, %p) file %p: %s\n", cb->cb, cb->info, file,
                dvdGetFilePath(file));
        }
    }
}

void dvdDumpPendingCancelCallbacks() {
    exiPrintf("Pending cancel callbacks: %d\n", nPendingCancelCbs);
    for(int i=0; i<nPendingCancelCbs; i++) {
        HackDvdOpenFile *file = (HackDvdOpenFile*)
            dvd_getFileByInfo(pendingCancelCb[i].info);
        exiPrintf("  %08x(0, %08x): %08x: %s\n",
            pendingCancelCb[i].cb, pendingCancelCb[i].info, file,
            dvdGetFilePath(file));
    }
}

void dvdDoPendingCancelCallbacks() {
    while(nPendingCancelCbs) {
        DVD_DPRINT("%d cancel callbacks\n", nPendingCancelCbs);
        DVDCBCallback cb = pendingCancelCb[0].cb;
        DVDFileInfo *info = pendingCancelCb[0].info;
        DVD_DPRINT("DVDCancelAsync callback %08X(0, %08X)\r\n",
            (u32)cb, (u32)info);
        cb(0, info);

        HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
        if(file) {
            fclose(file->file);
            dvd_removeFile(file);
        }

        memcpy(&pendingCancelCb[0], &pendingCancelCb[1],
            sizeof(DvdPendingCancelCb) * (DVD_MAX_CANCEL_CALLBACKS - 1));
        nPendingCancelCbs--;
    }
}

bool dvdAnyPendingCallbacks() {
    return (nPendingReadCbs + nPendingCancelCbs + nPendingStreamCbs) > 0;
}
