#include "main.h"

volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];
static u8 dvdThreadStack[DVD_THREAD_STACK_SIZE];

void initDvdHack() {
    exiPrintf("%s: init (r2=%08X r13=%08X)\n", __FUNCTION__,
        get_r2(), get_r13());
    memset((void*)dvdOpenFiles, 0, sizeof(HackDvdOpenFile) * DVD_MAX_OPEN_FILES);

    exiPrintf("%s: create alarm\n", __FUNCTION__);
    OSCreateAlarm(&dvdThreadAlarm);

    //LWP_MutexInit(&dvdMsgMutex, false);
    exiPrintf("%s: create threadQ\n", __FUNCTION__);
    OSInitThreadQueue(&dvdThreadQueue);

    exiPrintf("%s: create msgQ in\n", __FUNCTION__);
    OSInitMessageQueue(&hackDvdThreadMailIn,
        (OSMessage*)dvdMsgsIn, DVD_MAX_MSGS);

    exiPrintf("%s: create msgQ out\n", __FUNCTION__);
    OSInitMessageQueue(&hackDvdThreadMailOut,
        (OSMessage*)dvdMsgsOut, DVD_MAX_MSGS);

    exiPrintf("%s: create DVD thread\n", __FUNCTION__);
    if(!OSCreateThread(&hackDvdThread, hackDvdThreadMain,
        NULL, dvdThreadStack, DVD_THREAD_STACK_SIZE,
        DVD_THREAD_PRIO, 0)) {
            exiPrintf(" *** ERROR *** Failed to create DVD thread\n");
        return;
    }
    exiPrintf("DVD thread is %08X\n", &hackDvdThread);

    //wake the DVD thread up periodically
    //we don't have OSSetPeriodicAlarm though
    //so the alarm handler will have to set it again
    exiPrintf("%s: create DVD thread alarm\n", __FUNCTION__);
    OSSetAlarm(&dvdThreadAlarm, DVD_ALARM_PERIOD,
        dvdThreadAlarmCb);

    //OSYieldThread();
    exiPrintf("DVD thread setup OK, %08X, alarm=%08X\n",
        &hackDvdThread, &dvdThreadAlarm);

    //we need to start it too
    OSResumeThread(&hackDvdThread);
    OSYieldThread();
    exiPrintf("OSResumeThread OK for DVD\n");
}

int sendToDvdThread(HackDvdMsg *msg) {
    /** Called by another thread to send a message
     *  to the DVD thread.
     */
    while(!dvdThreadReady) OSYieldThread();
    //LWP_MutexLock(dvdMsgMutex);
    int next = (dvdMsgsInHead + 1) % DVD_MAX_MSGS;
    #if DVD_DEBUG
        exiPrintf("sendToDvdThread h=%d t=%d msg=%08X\n",
            dvdMsgsInHead, dvdMsgsInTail, &dvdMsgsIn[dvdMsgsInHead]);
    #endif
    if(next == dvdMsgsInTail) {
        exiPuts(" *** ERROR *** sendToDvdThread msg overflow\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EOVERFLOW;
    }
    msg->id = dvdCmdId++;
    void *buf = (void*)&dvdMsgsIn[dvdMsgsInHead];
    //void *buf = malloc(sizeof(HackDvdMsg));
    memcpy(buf, msg, sizeof(HackDvdMsg));
    bool r = OSSendMessage(&hackDvdThreadMailIn, (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    if(!r) {
        exiPrintf(" *** ERROR *** sendToDvdThread fail\n");
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    dvdMsgsInHead = next;
    #if DVD_DEBUG
        exiPrintf("wakeup DVD\n");
    #endif
    //LWP_ThreadSignal(dvdThreadQueue);
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}

int recvFromDvdThread(HackDvdMsg **msg, u32 flags) {
    /** Called by the DVD thread to receive the next message
     *  from another thread.
     */
    while(!dvdThreadReady) OSYieldThread();
    //LWP_MutexLock(dvdMsgMutex);
    int next = (dvdMsgsOutTail + 1) % DVD_MAX_MSGS;
    OSMessage m;
    //#if DVD_DEBUG
    //    exiPrintf("recvFromDvdThread h=%d t=%d msg=%08X\n",
    //        dvdMsgsOutHead, dvdMsgsOutTail, &dvdMsgsOut[dvdMsgsOutTail]);
    //#endif
    BOOL r = OSReceiveMessage(&hackDvdThreadMailOut, &m, flags);
    if(!r) {
        //LWP_MutexUnlock(dvdMsgMutex);
        return -EIO;
    }
    /*if(m != &dvdMsgsOut[dvdMsgsOutTail]) {
        exiPrintf(" *** ERROR *** recvFromDvdThread m=%08X out=%08X h=%d t=%d\n", m,
            dvdMmsgsOut[dvdMsgsOutTail], msgsOutHead, dvdMsgsOutTail);
    }*/
    *msg = m; //already pointing to the buffer
    dvdMsgsOutTail = next;
    //LWP_MutexUnlock(dvdMsgMutex);
    return 0;
}

int sendReadToDvdThread(DVDFileInfo *info, void *addr, uint length,
int offset, DVDCallback callback, int prio, bool async) {
    DVD_BUSY = 1;
    HackDvdMsg msg;
    HackDvdOpenFile *file = (HackDvdOpenFile*)dvd_getFileByInfo(info);
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
        exiPrintf("%s: sending...\n", __FUNCTION__);
    #endif
    OSYieldThread();
    //do {
        r = sendToDvdThread(&msg);
    //    OSYieldThread();
    //} while(r);
    if(r) {
        exiPrintf(" *** ERROR *** sendToDvdThread: %d\n", r);
        return r;
    }
    #if DVD_DEBUG
        exiPrintf("%s: sent\n", __FUNCTION__);
    #endif
    //OSYieldThread();
    if(!async) { //wait for reply
        #if DVD_DEBUG
            exiPrintf("Wait for DVD read for file %08X id %d\n",
                file, msg.id);
        #endif
        bool done = false;
        while(!done) {
            OSYieldThread();
            HackDvdMsg *resp;
            r = recvFromDvdThread(&resp, OS_MESSAGE_NOBLOCK);
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
    //OSYieldThread();
    #if DVD_DEBUG
        exiPrintf("DVD %ssync read finished, file %08X\n",
            async ? "a" : "", file);
    #endif
    return length;
}

