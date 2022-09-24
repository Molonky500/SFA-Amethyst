#include "main.h"

int _dvdDoRead(DVDFileInfo *file, void *addr, uint size);

vu32 canary1 = 0xFACEB007;
static vu32 canary3 = 0xB000B1E5;
mutex_t dvdMsgMutex;
lwpq_t dvdThreadQueue;
volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];

//DVD thread mailboxes
lwp_t hackDvdThread;
mqbox_t hackDvdThreadMailIn, hackDvdThreadMailOut;
volatile HackDvdMsg msgsIn[DVD_MAX_MSGS], msgsOut[DVD_MAX_MSGS];
int msgsInHead=0, msgsInTail=0, msgsOutHead=0, msgsOutTail=0;
syswd_t dvdThreadAlarm;
static u32 dvdCmdId=0;
vu32 canary2 = 0xABADBABE;
static vu32 canary4 = 0x2D0661E5;

HackDvdOpenFile* getFileByInfo(DVDFileInfo *info) {
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].info == info) {
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    exiPrintf(" *** ERROR *** Can't find fileinfo %08X!\n", info);
    return NULL;
}
HackDvdOpenFile* getFileByHandle(FILE *file) {
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].file == file) {
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    exiPrintf(" *** ERROR *** Can't find FILE* %08X!\n", file);
    return NULL;
}
HackDvdOpenFile* addFile(DVDFileInfo *info, FILE *file) {
    if(!file) {
        exiPuts(" *** ERROR *** adding NULL file!\n");
        while(1);
    }
    for(int i=0; i<DVD_MAX_OPEN_FILES; i++) {
        if(dvdOpenFiles[i].info == NULL) {
            dvdOpenFiles[i].info = info;
            dvdOpenFiles[i].file = file;
            exiPrintf("addFile(%08X, %08X) slot %d %08X\n",
                info, file, i, &dvdOpenFiles[i]);
            return (HackDvdOpenFile*)&dvdOpenFiles[i];
        }
    }
    exiPuts(" *** ERROR *** Too many open files!\n");
    return NULL;
}
void removeFile(HackDvdOpenFile* file) {
    file->info = NULL;
    file->file = NULL;
}

int sendFromDvdThread(HackDvdMsg *msg) {
    /** Called by the DVD thread to send a message
     *  to another thread.
     */
    //LWP_MutexLock(dvdMsgMutex);
    int next = (msgsOutHead + 1) % DVD_MAX_MSGS;
    if(next == msgsOutTail) {
        exiPuts(" *** ERROR *** sendFromDvdThread msg overflow\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EOVERFLOW;
    }
    //exiPrintf("sendFromDvdThread h=%d t=%d msg=%08X\n",
    //    msgsOutHead, msgsOutTail, &msgsOut[msgsOutHead]);
    void *buf = (void*)&msgsOut[msgsOutHead];
    //void *buf = malloc(sizeof(HackDvdMsg));
    memcpy(buf, msg, sizeof(HackDvdMsg));
    bool r = MQ_Send(hackDvdThreadMailOut, (mqmsg_t)buf, MQ_MSG_NOBLOCK);
    if(!r) {
        exiPrintf(" *** ERROR *** sendFromDvdThread fail\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    msgsOutHead = next;
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}
int sendToDvdThread(HackDvdMsg *msg) {
    /** Called by another thread to send a message
     *  to the DVD thread.
     */
    //LWP_MutexLock(dvdMsgMutex);
    if(!hackDvdThreadMailIn) {
        exiPrintf(" *** ERROR *** hackDvdThreadMailIn is 0\n");
        return -EIO;
    }
    int next = (msgsInHead + 1) % DVD_MAX_MSGS;
    #if DVD_DEBUG
        exiPrintf("sendToDvdThread h=%d t=%d msg=%08X\n",
            msgsInHead, msgsInTail, &msgsIn[msgsInHead]);
    #endif
    if(next == msgsInTail) {
        exiPuts(" *** ERROR *** sendToDvdThread msg overflow\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EOVERFLOW;
    }
    msg->id = dvdCmdId++;
    void *buf = (void*)&msgsIn[msgsInHead];
    //void *buf = malloc(sizeof(HackDvdMsg));
    memcpy(buf, msg, sizeof(HackDvdMsg));
    bool r = MQ_Send(hackDvdThreadMailIn, (mqmsg_t)buf, MQ_MSG_NOBLOCK);
    if(!r) {
        exiPrintf(" *** ERROR *** sendToDvdThread fail\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    msgsInHead = next;
    #if DVD_DEBUG
        exiPrintf("wakeup DVD, q=%08X\n", dvdThreadQueue);
    #endif
    //LWP_ThreadSignal(dvdThreadQueue);
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}
int recvToDvdThread(HackDvdMsg **msg, u32 flags) {
    /** Called by the DVD thread to receive the next message
     *  from another thread.
     */
    //LWP_MutexLock(dvdMsgMutex);
    int next = (msgsInTail + 1) % DVD_MAX_MSGS;
    mqmsg_t m;
    //exiPrintf("recvToDvdThread h=%d t=%d msg=%08X\n",
    //    msgsInHead, msgsInTail, &msgsIn[msgsInTail]);
    BOOL r = MQ_Receive(hackDvdThreadMailIn, &m, flags);
    if(!r) {
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    *msg = m; //already pointing to the buffer
    msgsInTail = next;
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}
int recvFromDvdThread(HackDvdMsg **msg, u32 flags) {
    /** Called by the DVD thread to receive the next message
     *  from another thread.
     */
    //LWP_MutexLock(dvdMsgMutex);
    int next = (msgsOutTail + 1) % DVD_MAX_MSGS;
    mqmsg_t m;
    #if DVD_DEBUG
        exiPrintf("recvFromDvdThread h=%d t=%d msg=%08X\n",
            msgsOutHead, msgsOutTail, &msgsOut[msgsOutTail]);
    #endif
    BOOL r = MQ_Receive(hackDvdThreadMailOut, &m, flags);
    if(!r) {
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    /*if(m != &msgsOut[msgsOutTail]) {
        exiPrintf(" *** ERROR *** recvFromDvdThread m=%08X out=%08X h=%d t=%d\n", m,
            &msgsOut[msgsOutTail], msgsOutHead, msgsOutTail);
    }*/
    *msg = m; //already pointing to the buffer
    msgsOutTail = next;
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}

void dvdThreadAlarmCb(syswd_t alarm, void *arg) {
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
    LWP_ThreadSignal(dvdThreadQueue);
}

void* hackDvdThreadMain(void *param) {
    /** Main thread for async DVD emulation.
     */
    //exiPuts("DVD thread online\n");
    int err = 0;
    bool run = true;
    while(run) {
        HackDvdMsg *msg = NULL;

        while(true) {
            #if DVD_DEBUG
                exiPrintf("DVD thread waiting, q=%08X\n", dvdThreadQueue);
            #endif
            LWP_YieldThread();
            LWP_ThreadSleep(dvdThreadQueue);
            err = recvToDvdThread(&msg, MQ_MSG_NOBLOCK);
            if(!err) break;
            #if DVD_DEBUG
                exiPuts("DVD thread awake\n");
            #endif
            LWP_YieldThread();
        }

        if(err) {
            //happens if DVD thread priority is too low.
            exiPrintf(" *** ERROR *** recvToDvdThread: %d\n", err);
            run = false;
            break;
            //LWP_YieldThread();
            //continue;
        }
        #if DVD_DEBUG
            exiPrintf("DVD thread cmd %d id %d\n", msg->cmd, msg->id);
        #endif

        switch(msg->cmd) {
            case DVDCMD_SHUTDOWN: {
                run = false;
                break;
            }
            case DVDCMD_READ: {
                //XXX prio
                HackDvdOpenFile *file = msg->read.file;
                void *addr = msg->read.addr;
                int length = msg->read.length;
                uint offset = msg->read.offset;
                DVDCallback callback = msg->read.callback;

                #if DVD_DEBUG
                    exiPrintf("DVD thread read f=%08X->%08X o=%08X a=%08X l=%08X (end %08X)\n",
                        file, file->file, offset, addr, length, addr+length);
                #endif
                if(!file) break;
                fseek(file->file, offset, SEEK_SET);
                file->info->cb.offset = offset;
                int r = _dvdDoRead(file->info, addr, length);
                #if DVD_DEBUG
                    exiPrintf("DVD thread res=%d callback %08X\n",
                        r, callback);
                #endif
                LWP_YieldThread();

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
                if(callback) {
                    #if DVD_DEBUG
                        exiPrintf("DVD CB for file %08X: %08X(%d)\n",
                            file, callback, r);
                    #endif
                    switchToGame();
                    callback(r, file->info);
                    switchToOgc();
                    #if DVD_DEBUG
                        exiPrintf("DVD CB for file %08X done\n", file);
                    #endif
                }
                #if DVD_DEBUG
                    exiPuts("DVD thread read done\n");
                #endif
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

void initDvdHack() {
    memset((void*)dvdOpenFiles, 0, sizeof(HackDvdOpenFile) * DVD_MAX_OPEN_FILES);
    exiPrintf("hackDvdThread=%08X hackDvdThreadMailIn=%08X "
        "hackDvdThreadMailOut=%08X msgsIn=%08X msgsOut=%08X "
        "msgsInHead=%08X",
        &hackDvdThread, &hackDvdThreadMailIn,
        &hackDvdThreadMailOut, msgsIn, msgsOut,
        &msgsInHead);

    int r = SYS_CreateAlarm(&dvdThreadAlarm);
    if(r) {
        exiPrintf(" *** ERROR *** SYS_CreateAlarm %d\n", r);
    }

    //LWP_MutexInit(&dvdMsgMutex, false);
    r = LWP_InitQueue(&dvdThreadQueue);
    if(r) {
        exiPrintf(" *** ERROR *** LWP_InitQueue %d\n", r);
    }

    r = MQ_Init(&hackDvdThreadMailIn,  DVD_MAX_MSGS);
    if(r) {
        exiPrintf(" *** ERROR *** MQ_Init %d\n", r);
    }

    r = MQ_Init(&hackDvdThreadMailOut, DVD_MAX_MSGS);
    if(r) {
        exiPrintf(" *** ERROR *** MQ_Init %d\n", r);
    }

    r = LWP_CreateThread(&hackDvdThread, hackDvdThreadMain,
        NULL, NULL, 65536, DVD_THREAD_PRIO);
    if(r) {
        exiPrintf(" *** ERROR *** Failed to create DVD thread: %d\n", r);
    }

    //wake the DVD thread up periodically
    struct timespec tStart, tPeriod;
    tStart .tv_sec  = 0;
    tStart .tv_nsec = 200000;
    tPeriod.tv_sec  = 0;
    tPeriod.tv_nsec = 200000;
    SYS_SetPeriodicAlarm(dvdThreadAlarm,
        &tStart, &tPeriod, dvdThreadAlarmCb, NULL);

    LWP_YieldThread();
    exiPrintf("DVD thread setup OK, %08X, alarm=%08X\n",
        hackDvdThread, &dvdThreadAlarm);
}


bool DVDOpen_hook(const char *path, DVDFileInfo *info) {
    switchToOgc();

    char newPath[4096];
    snprintf(newPath, sizeof(newPath), "%s/files/%s",
        gameRootDir, path);
//#if DVD_DEBUG
    exiPrintf("DVDOpen(%s, %08X)... ", newPath, info);
//#endif
    FILE *file = fopen(newPath, "rb");
    if(!file) {
        int err = errno;
        exiPrintf(" *** ERROR *** DVDOpen(%s, %08X) FAILED: %d\n",
            newPath, info, err);
        switchToGame();
        return false;
    }
    addFile(info, file);
    info->startAddr = 0;
    info->callback = NULL;
    info->cb.state = 0;

    fseek(file, 0, SEEK_END);
    info->length = ftell(file);
    fseek(file, 0, SEEK_SET);

//#if DVD_DEBUG
    exiPrintf("file=0x%08X size=%d\n", file, info->length);
//#endif

    LWP_YieldThread();
    switchToGame();
    return true;
}

int _dvdDoRead(DVDFileInfo *info, void *addr, uint size) {
    //no switch to ogc here because this is called from
    //functions that have already switched
    int r = 0;
    int nRead = 0;
    DVD_BUSY = 1; //DVD drive is busy

#if DVD_DEBUG
    exiPrintf("DVDRead(%08X %08X %08X)\n", info, addr, size);
#endif

    HackDvdOpenFile *file = (HackDvdOpenFile*)getFileByInfo(info);
    if(!file) return -EMFILE;
    while(nRead < size) {
        //memset(addr, 0xAAAAAAAA, MIN(DVD_SECTOR_SIZE, size-nRead));
        #if DVD_DEBUG
            exiPrintf("DVD fread(a=%08X s=%08X f=%08X->%08X)\n",
                addr, MIN(DVD_SECTOR_SIZE, size-nRead), file, file->file);
        #endif
        r = fread(addr, 1, MIN(DVD_SECTOR_SIZE, size-nRead), file->file);
        #if DVD_DEBUG
            exiPrintf("fread: %d\n", r);
        #endif
        if(r <= 0) break;

        DCInvalidateRange(addr, r);
        nRead += r;
        addr  += r;
        info->cb.offset += r;
        //the game dislikes us reading too fast
        //for(int i=0; i<60; i++) LWP_YieldThread();
        //LWP_YieldThread();

        if(LWP_GetSelf() == hackDvdThread) {
            #if DVD_DEBUG
                exiPrintf("DVD thread pause, q=%08X\n", dvdThreadQueue);
            #endif
            LWP_ThreadSleep(dvdThreadQueue);
            #if DVD_DEBUG
                exiPuts("DVD thread resume\n");
            #endif
        }
    }
    if(nRead < size) {
#if DVD_DEBUG
        exiPrintf("DVDRead(%08X): want %d but got %d\n",
            file, size, nRead);
#endif
    }
    else {
#if DVD_DEBUG
        exiPrintf("DVDRead(%08X -> %08X): %d: %08X %08X %08X %08X...\n",
            file, (u32)addr, nRead,
            *(u32*)addr,
            *(u32*)(addr+4),
            *(u32*)(addr+8),
            *(u32*)(addr+12) );
#endif
    }
    DVD_BUSY = 0;
    return (r >= 0) ? nRead : r;
}

int _sendReadToDvdThread(DVDFileInfo *info, void *addr, uint length,
int offset, DVDCallback callback, int prio, bool async) {
    DVD_BUSY = 1;
    HackDvdMsg msg;
    HackDvdOpenFile *file = (HackDvdOpenFile*)getFileByInfo(info);
    msg.cmd = DVDCMD_READ;
    //sendToDvdThread fills in msg.id
    msg.read.file = file;
    msg.read.addr = addr;
    msg.read.length = length;
    msg.read.offset = offset;
    msg.read.callback = callback;
    msg.read.prio = prio;
    msg.read.async = async;
    int r = 0;
    #if DVD_DEBUG
        exiPrintf("_sendReadToDvdThread: sending...\n");
    #endif
    LWP_YieldThread();
    //do {
        r = sendToDvdThread(&msg);
    //    LWP_YieldThread();
    //} while(r);
    if(r) {
        exiPrintf(" *** ERROR *** sendToDvdThread: %d\n", r);
        return r;
    }
    #if DVD_DEBUG
        exiPrintf("_sendReadToDvdThread: sent\n");
    #endif
    //LWP_YieldThread();
    if(!async) { //wait for reply
        #if DVD_DEBUG
            exiPrintf("Wait for DVD read for file %08X id %d\n",
                file, msg.id);
        #endif
        bool done = false;
        while(!done) {
            LWP_YieldThread();
            HackDvdMsg *resp;
            r = recvFromDvdThread(&resp, MQ_MSG_NOBLOCK);
            if(r) continue;
            //if(r) {
            //    exiPrintf(" *** ERROR *** recvFromDvdThread: %d\n", r);
            //    break;
            //}
            #if DVD_DEBUG
                exiPrintf("recvFromDvdThread m=%d id=%d file=%08X\n",
                    resp->cmd, resp->id, resp->read.file);
            #endif
            if(resp->id == msg.id) done = true;
            //free(resp);
        }
    }
    //LWP_YieldThread();
    #if DVD_DEBUG
        exiPrintf("DVD %ssync read finished, file %08X\n",
            async ? "a" : "", file);
    #endif
    return length;
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
    int r = _sendReadToDvdThread(info, addr, size,
        offset, NULL, 0, false);
    LWP_YieldThread();
    switchToGame();
    return r;
}

int DVDClose_hook(DVDFileInfo *info) {
    switchToOgc();
#if DVD_DEBUG
    exiPrintf("DVDClose(%08X)\n", info);
#endif
    LWP_YieldThread();
    HackDvdOpenFile *file = (HackDvdOpenFile*)getFileByInfo(info);
    if(file) {
        fclose(file->file);
        removeFile(file);
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
    int r = _sendReadToDvdThread(info, addr, length,
        offset, NULL, prio, false);

    LWP_YieldThread();
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

    int r = _sendReadToDvdThread(info, addr, length,
        offset, callback, prio, true);
    if(r <= 0) {
        exiPrintf(" *** ERROR *** DVDReadAsyncPrio send failed: %d\n", r);
    }
    LWP_YieldThread();
    switchToGame();
    return (r > 0);
}

BOOL DVDPrepareStreamAsync_hook(DVDFileInfo *fInfo, u32 length,
u32 offset, DVDCallback callback) {
    callback(0, fInfo);
    LWP_YieldThread();
    return true;
}

BOOL DVDCancelStreamAsync_hook(DVDCommandBlock *block,
DVDCBCallback callback) {
    callback(0, block);
    LWP_YieldThread();
    return true;
}
