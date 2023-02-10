#include "main.h"

volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];
static u8 dvdThreadStack[DVD_THREAD_STACK_SIZE];
OSMutex dvdMsgMutex;

void initDvdHack() {
    exiPrintf("%s: init (r2=%08X r13=%08X)\r\n", __FUNCTION__,
        get_r2(), get_r13());
    memset((void*)dvdOpenFiles, 0, sizeof(HackDvdOpenFile) * DVD_MAX_OPEN_FILES);
    memset((void*)dvdMsgsIn,  0, sizeof(HackDvdMsg) * DVD_MAX_MSGS);
    memset((void*)dvdMsgsOut, 0, sizeof(HackDvdMsg) * DVD_MAX_MSGS);
    OSInitMutex(&dvdMsgMutex);

    exiPrintf("%s: create alarm\r\n", __FUNCTION__);
    OSCreateAlarm(&dvdThreadAlarm);

    exiPrintf("%s: create threadQ\r\n", __FUNCTION__);
    OSInitThreadQueue(&dvdThreadQueue);

    exiPrintf("%s: create msgQ in\r\n", __FUNCTION__);
    OSInitMessageQueue(&hackDvdThreadMailIn,
        hackDvdMailboxIn, DVD_MAX_MSGS);

    exiPrintf("%s: create msgQ out\r\n", __FUNCTION__);
    OSInitMessageQueue(&hackDvdThreadMailOut,
        hackDvdMailboxOut, DVD_MAX_MSGS);

    exiPrintf("%s: create DVD thread\r\n", __FUNCTION__);
    if(!OSCreateThread(&hackDvdThread, hackDvdThreadMain,
        NULL, dvdThreadStack+DVD_THREAD_STACK_SIZE,
        DVD_THREAD_STACK_SIZE, DVD_THREAD_PRIO, 0)) {
            PANIC("Failed to create DVD thread\r\n");
        return;
    }
    exiPrintf("DVD thread is %08X, queue=%08X, %08X, buf=%08X, %08X\r\n",
        &hackDvdThread,
        &hackDvdThreadMailIn, &hackDvdThreadMailOut,
        dvdMsgsIn, dvdMsgsOut);

    //wake the DVD thread up periodically
    //we don't have OSSetPeriodicAlarm though
    //so the alarm handler will have to set it again
    exiPrintf("%s: create DVD thread alarm\r\n", __FUNCTION__);
    OSSetAlarm(&dvdThreadAlarm, DVD_ALARM_PERIOD,
        dvdThreadAlarmCb);

    //OSYieldThread();
    exiPrintf("DVD thread setup OK, %08X, alarm=%08X\r\n",
        &hackDvdThread, &dvdThreadAlarm);

    //we need to start it too
    OSResumeThread(&hackDvdThread);
    OSYieldThread();
    exiPrintf("OSResumeThread OK for DVD\r\n");
}

int sendToDvdThread(HackDvdMsg *msg) {
    /** Called by another thread to send a message
     *  to the DVD thread.
     */
    while(!dvdThreadReady) OSYieldThread();
    OSLockMutex(&dvdMsgMutex);
    int next = (dvdMsgsInHead + 1) % DVD_MAX_MSGS;
    DVD_DPRINT("sendToDvdThread h=%d t=%d msg=%08X->%08X (cmd %02X)\r\n",
        dvdMsgsInHead, dvdMsgsInTail, (u32)msg,
        (u32)&dvdMsgsIn[dvdMsgsInHead],
        msg->cmd);
    if(next == dvdMsgsInTail) {
        exiPuts(" *** ERROR *** sendToDvdThread msg overflow\r\n");
        OSUnlockMutex(&dvdMsgMutex);
        return -EOVERFLOW;
    }
    msg->id = dvdCmdId++;
    void *buf = (void*)&dvdMsgsIn[dvdMsgsInHead];
    //void *buf = malloc(sizeof(HackDvdMsg));
    memcpy(buf, msg, sizeof(HackDvdMsg));
    //send the pointer to the message
    bool r = OSSendMessage(&hackDvdThreadMailIn, (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    if(!r) {
        exiPrintf(" *** ERROR *** sendToDvdThread fail\r\n");
        OSUnlockMutex(&dvdMsgMutex);
        return -EIO;
    }
    dvdMsgsInHead = next;
    DVD_DPRINT("wakeup DVD\r\n");
    //LWP_ThreadSignal(dvdThreadQueue);
    OSUnlockMutex(&dvdMsgMutex);
    return 0;
}

int recvFromDvdThread(HackDvdMsg **msg, u32 flags) {
    /** Called by the DVD thread to receive the next message
     *  from another thread.
     */
    while(!dvdThreadReady) OSYieldThread();
    OSLockMutex(&dvdMsgMutex);
    int next = (dvdMsgsOutTail + 1) % DVD_MAX_MSGS;
    OSMessage m;
    //    DVD_DPRINT("recvFromDvdThread h=%d t=%d msg=%08X\n",
    //        dvdMsgsOutHead, dvdMsgsOutTail, &dvdMsgsOut[dvdMsgsOutTail]);
    BOOL r = OSReceiveMessage(&hackDvdThreadMailOut, &m, flags);
    if(!r) {
        OSUnlockMutex(&dvdMsgMutex);
        return -EIO;
    }
    /*if(m != &dvdMsgsOut[dvdMsgsOutTail]) {
        exiPrintf(" *** ERROR *** recvFromDvdThread m=%08X out=%08X h=%d t=%d\n", m,
            dvdMmsgsOut[dvdMsgsOutTail], msgsOutHead, dvdMsgsOutTail);
    }*/
    *msg = m; //already pointing to the buffer
    dvdMsgsOutTail = next;
    OSUnlockMutex(&dvdMsgMutex);
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
    DVD_DPRINT("%s: sending...\r\n", __FUNCTION__);
    //OSYieldThread();
    //do {
        r = sendToDvdThread(&msg);
    //    OSYieldThread();
    //} while(r);
    if(r) {
        exiPrintf(" *** ERROR *** sendToDvdThread: %d\r\n", r);
        return r;
    }
    DVD_DPRINT("%s: sent\n", __FUNCTION__);
    //OSYieldThread();
    if(!async) { //wait for reply
        DVD_DPRINT("Wait for DVD read for file %08X id %d\r\n",
            file, msg.id);
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
            DVD_DPRINT("recvFromDvdThread m=%d id=%d file=%08X\r\n",
                resp->cmd, resp->id, resp->read.file);
            if(resp->id == msg.id) done = true;
            //free(resp);
        }
    }
    //OSYieldThread();
    DVD_DPRINT("DVD %ssync read finished, file %08X\r\n",
        async ? "a" : "", file);
    return length;
}

