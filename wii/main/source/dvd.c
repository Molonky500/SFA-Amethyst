#include "main.h"

volatile HackDvdOpenFile dvdOpenFiles[DVD_MAX_OPEN_FILES];
static u8 dvdThreadStack[DVD_THREAD_STACK_SIZE];

void initDvdHack() {
    int r;
    exiPrintf("%s: init (r2=%08X r13=%08X)\n", __FUNCTION__,
        get_r2(), get_r13());
    memset((void*)dvdOpenFiles, 0, sizeof(HackDvdOpenFile) * DVD_MAX_OPEN_FILES);

    switchToGame();
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
        switchToOgc();
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
    switchToOgc();
}
