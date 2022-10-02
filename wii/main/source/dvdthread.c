#include "main.h"

vu32 canary1 = 0xFACEB007;
static vu32 canary3 = 0xB000B1E5;
volatile bool dvdThreadReady = false;
OSThreadQueue dvdThreadQueue;
OSThread hackDvdThread;
OSMutex dvdFileInfoMutex;

//DVD thread mailboxes
//these are the OSMessageQueue objects that manage messages
OSMessageQueue hackDvdThreadMailIn, hackDvdThreadMailOut;
//these store the actual OSMessage objects (which are
//pointers to the actual message)
OSMessage hackDvdMailboxIn[DVD_MAX_MSGS], hackDvdMailboxOut[DVD_MAX_MSGS];
//these store the actual messages, pointed to by above
volatile HackDvdMsg dvdMsgsIn[DVD_MAX_MSGS], dvdMsgsOut[DVD_MAX_MSGS];
//these keep track of what buffer space is available
int dvdMsgsInHead=0, dvdMsgsInTail=0, dvdMsgsOutHead=0, dvdMsgsOutTail=0;

OSAlarm dvdThreadAlarm;
u32 dvdCmdId=0;
vu32 canary2 = 0xABADBABE;
static vu32 canary4 = 0x2D0661E5;

HackDvdOpenFile* dvd_getFileByInfo(DVDFileInfo *info) {
    OSLockMutex(&dvdFileInfoMutex);
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].info == info) {
            OSUnlockMutex(&dvdFileInfoMutex);
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    //it's okay if the file isn't found.
    OSUnlockMutex(&dvdFileInfoMutex);
    return NULL;
}
HackDvdOpenFile* dvd_getFileByHandle(FILE *file) {
    OSLockMutex(&dvdFileInfoMutex);
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].file == file) {
            OSUnlockMutex(&dvdFileInfoMutex);
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    exiPrintf(" *** ERROR *** Can't find FILE* %08X!\n", file);
    OSUnlockMutex(&dvdFileInfoMutex);
    return NULL;
}
HackDvdOpenFile* dvd_addFile(DVDFileInfo *info, FILE *file, const char *path) {
    if(!file) {
        exiPuts(" *** ERROR *** adding NULL file!\n");
        while(1);
    }
    OSLockMutex(&dvdFileInfoMutex);
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].info == NULL) {
            dvdOpenFiles[i].info = info;
            dvdOpenFiles[i].file = file;
            #if DVD_DEBUG
                strncpy(dvdOpenFiles[i].path, path, 256);
            #endif
            DVD_DPRINT("dvd_addFile(%08X, %08X) slot %d %08X\n",
                info, file, i, &dvdOpenFiles[i]);
            OSUnlockMutex(&dvdFileInfoMutex);
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    exiPuts(" *** ERROR *** Too many open files!\n");
    OSUnlockMutex(&dvdFileInfoMutex);
    return NULL;
}
void dvd_removeFile(HackDvdOpenFile* file) {
    file->info = NULL;
    file->file = NULL;
}

int sendFromDvdThread(HackDvdMsg *msg) {
    /** Called by the DVD thread to send a message
     *  to another thread.
     */
    OSLockMutex(&dvdMsgMutex);
    int next = (dvdMsgsOutHead + 1) % DVD_MAX_MSGS;
    if(next == dvdMsgsOutTail) {
        exiPuts(" *** ERROR *** sendFromDvdThread msg overflow\n");
        OSUnlockMutex(&dvdMsgMutex);
        return -EOVERFLOW;
    }
    //exiPrintf("sendFromDvdThread h=%d t=%d msg=%08X\n",
    //    dvdMsgsOutHead, dvdMsgsOutTail, &dvdMsgsOut[dvdMsgsOutHead]);
    void *buf = (void*)&dvdMsgsOut[dvdMsgsOutHead];
    //void *buf = malloc(sizeof(HackDvdMsg));
    memcpy(buf, msg, sizeof(HackDvdMsg));
    bool r = OSSendMessage(&hackDvdThreadMailOut, (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    if(!r) {
        exiPrintf(" *** ERROR *** sendFromDvdThread fail\n");
        OSUnlockMutex(&dvdMsgMutex);
        return -EIO;
    }
    dvdMsgsOutHead = next;
    OSUnlockMutex(&dvdMsgMutex);
    OSWakeupThread(&dvdThreadQueue);
    return 0;
}

int recvToDvdThread(HackDvdMsg **msg, u32 flags) {
    /** Called by the DVD thread to receive the next message
     *  from another thread.
     */
    OSLockMutex(&dvdMsgMutex);
    int next = (dvdMsgsInTail + 1) % DVD_MAX_MSGS;
    OSMessage m;
    //exiPrintf("recvToDvdThread h=%d t=%d msg=%08X\n",
    //    dvdMsgsInHead, dvdMsgsInTail, &dvdMsgsIn[dvdMsgsInTail]);
    BOOL r = OSReceiveMessage(&hackDvdThreadMailIn, &m, flags);
    if(!r) {
        OSUnlockMutex(&dvdMsgMutex);
        return -EIO;
    }
    *msg = (HackDvdMsg*)m; //m points to buf
    DVD_DPRINT("recvToDvdThread m=%08X -> %08X\n",
        (u32)m, (u32)*msg);
    dvdMsgsInTail = next;
    OSUnlockMutex(&dvdMsgMutex);
    return 0;
}

int _dvdDoRead(DVDFileInfo *info, void *addr, uint size) {
    //no switch to ogc here because this is called from
    //functions that have already switched
    int r = 0;
    int nRead = 0;
    HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
    if(!file) {
        printf(" *** ERROR *** too many open files\n");
        return -EMFILE;
    }
    #if DVD_DEBUG
        DVD_DPRINT("DVDRead(f=%08X dst=%08X len=%08X) (%s)\n",
            info, addr, size, file->path);
    #endif

    void *readDest = addr;
    while(nRead < size) {
        DCInvalidateRange(readDest, size-nRead);
        memset(readDest, 0xBBBBBBBB, MIN(DVD_SECTOR_SIZE, size-nRead));
        DVD_DPRINT("DVD fread(a=%08X s=%08X f=%08X->%08X)\n",
            readDest, MIN(DVD_SECTOR_SIZE, size-nRead), file, file->file);
        r = fread(readDest, 1, MIN(DVD_SECTOR_SIZE, size-nRead), file->file);
        DVD_DPRINT("fread: %d\n", r);
        if(r <= 0) break;
        DCFlushRange(readDest, r);

        nRead    += r;
        readDest += r;
        info->cb.offset += r;
        //the game dislikes us reading too fast
        //OSYieldThread();

        /*if(OSGetCurrentThread() == &hackDvdThread) {
            DVD_DPRINT("DVD thread pause, q=%08X\n", dvdThreadQueue);
            OSSleepThread(&dvdThreadQueue);
            DVD_DPRINT("DVD thread resume\n");
        }*/
    }
    if(nRead < size) {
        DVD_DPRINT("DVDRead(%08X): want %d but got %d\n",
            file, size, nRead);
    }
    else {
        DVD_DPRINT("DVDRead(%08X -> %08X): %d: %08X %08X %08X %08X...\n",
            file, (u32)addr, nRead,
            *(u32*)addr,
            *(u32*)(addr+4),
            *(u32*)(addr+8),
            *(u32*)(addr+12) );
    }
    return (r >= 0) ? nRead : r;
}

void dvdThreadAlarmCb(OSAlarm *alarm, OSContext *ctx) {
    //#if DVD_DEBUG
    //    exiPrintf("DVD alarm CB, q=%08X\n", dvdThreadQueue);
    //#endif
    static vu32 canary5 = 0xA55FACE5;
    if(canary1 != 0xFACEB007 || canary2 != 0xABADBABE) {
        exiPrintf("CANARY FAIL %08X %08X\n", canary1, canary2);
    }
    if(canary3 != 0xB000B1E5 || canary4 != 0x2D0661E5) {
        exiPrintf("S-CANARY FAIL %08X %08X\n", canary3, canary4);
    }
    if(canary5 != 0xA55FACE5) {
        exiPrintf("L-CANARY FAIL %08X\n", canary5);
    }

    //set alarm again because we don't have OSSetPeriodicAlarm
    OSSetAlarm(&dvdThreadAlarm, DVD_ALARM_PERIOD,
        dvdThreadAlarmCb);
    OSWakeupThread(&dvdThreadQueue);
}

void dvdIdle() {
    int wasBusy = DVD_BUSY;
    DVD_BUSY = 0;
    OSYieldThread();
    DVD_BUSY = wasBusy;
}

void* hackDvdThreadMain(void *param) {
    /** Main thread for async DVD emulation.
     */
    __UnmaskIrq(IM_PI_ACR);
    OSEnableInterrupts();
    exiPuts("DVD thread online\n");

    if(fatInitDefault()) {
        exiPrintf("DVD FAT init OK\n");
    }
    else {
        exiPrintf("DVD FAT init FAIL\n");
        return NULL;
    }

    OSInitMutex(&dvdFileInfoMutex);
    dvdThreadReady = true;

    int err = 0;
    bool run = true;
    while(run) {
        HackDvdMsg *msg = NULL;

        while(true) {
            #if DVD_DEBUG
                //exiPrintf("DVD thread waiting, q=%08X\n", dvdThreadQueue);
            #endif
            DVD_BUSY = 0;
            dvdIdle();
            OSSleepThread(&dvdThreadQueue);
            err = recvToDvdThread(&msg, OS_MESSAGE_NOBLOCK);
            if(!err) break;
            #if DVD_DEBUG
                //exiPuts("DVD thread awake\n");
            #endif
        }

        if(err) {
            //happens if DVD thread priority is too low.
            exiPrintf(" *** ERROR *** recvToDvdThread: %d\n", err);
            run = false;
            break;
            //dvdIdle();
            //continue;
        }
        DVD_BUSY = 1; //DVD drive is busy
        DVD_DPRINT("DVD thread cmd 0x%X id 0x%X\n", msg->cmd, msg->id);

        switch(msg->cmd) {
            case DVDCMD_SHUTDOWN: {
                run = false;
                break;
            }
            case DVDCMD_READ: {
                //XXX prio
                HackDvdOpenFile *file = msg->read.file;
                void *addr  = msg->read.addr;
                int  length = msg->read.length;
                uint offset = msg->read.offset;
                DVDCallback callback = msg->read.callback;

                DVD_DPRINT("DVD thread read f=%08X->%08X o=%08X a=%08X l=%08X (end %08X)\n",
                    file, file->file, offset, addr, length, addr+length);
                if(!file) break;

                fseek(file->file, offset, SEEK_SET);
                file->info->cb.offset = offset;
                int r = _dvdDoRead(file->info, addr, length);
                DVD_DPRINT("DVD thread res=%d callback %08X\n",
                    r, callback);
                dvdIdle();

                if(!msg->read.async) {
                    HackDvdMsg mRead;
                    mRead.cmd = DVDCMD_READ;
                    mRead.id = msg->id;
                    mRead.read.file = file;
                    mRead.read.addr = addr;
                    mRead.read.length = r;
                    mRead.read.offset = offset;
                    mRead.read.callback = callback;
                    mRead.read.prio = msg->read.prio;
                    int err = sendFromDvdThread(&mRead);
                    if(err) {
                        exiPrintf(" *** ERROR *** sendFromDvdThread: %d\n", err);
                    }
                }
                /*if(file->info->cb.callback) {
                    DVD_DPRINT("DVDCB callback for file %08X: %08X(%d, %08X)\n",
                        file, file->info->cb.callback, r, &file->info->cb);
                    //void DVDCBCallback(undefined4 param, DVDCommandBlock * block)
                    file->info->cb.callback(r, &file->info->cb);
                    DVD_DPRINT("DVDCB callback for file %08X done\n", file);
                }*/

                if(callback) {
                    DVD_DPRINT("DVD callback for file %08X: %08X(%d, %08X)\n",
                        file, callback, r, file->info);
                    callback(r, file->info);
                    DVD_DPRINT("DVD callback for file %08X done\n", file);
                }

                DVD_DPRINT("DVD thread read done\n");
                break;
            }
            default: {
                exiPrintf(" *** ERROR *** DVD thread unknown cmd %d\n", msg->cmd);
                run = false;
                break;
            }
        }
        //free(msg);
    }
    exiPuts("DVD thread shutdown\n");
    HackDvdMsg mShutdown;
    mShutdown.cmd = DVDCMD_SHUTDOWN;
    sendFromDvdThread(&mShutdown);
    return NULL;
}
