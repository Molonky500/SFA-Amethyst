#include "main.h"

u32 lwpFlagsToDolFlags(u32 flags) {
    u32 dolFlags = 0;
    if(!(flags & MQ_MSG_NOBLOCK)) dolFlags |= OS_MESSAGE_BLOCK;
    return dolFlags;
}

s32 MQ_Init_dol(mqbox_t *mqbox,u32 count) {
    OSMessageQueue *dolQ = malloc(sizeof(OSMessageQueue) + (
        sizeof(OSMessage) * count));
    if(!dolQ) {
        exiPrintf(" *** ERROR *** malloc failed for %d msgs in %s\n",
            count, __FUNCTION__);
        return -1;
    }
    //we use the memory immediately following the queue itself
    //to store the messages, so we only need to do one alloc.
    OSInitMessageQueue(dolQ, (OSMessage*)&dolQ[1], count);
    return 0;
}

void MQ_Close_dol(mqbox_t mqbox) {
    if(!PTR_VALID(mqbox)) {
        exiPrintf(" *** ERROR *** Invalid mqbox %08X in %s\n",
            mqbox, __FUNCTION__);
        return;
    }
    free((OSMessageQueue*)mqbox);
}

BOOL MQ_Send_dol(mqbox_t mqbox,mqmsg_t msg,u32 flags) {
    if(!PTR_VALID(mqbox)) {
        exiPrintf(" *** ERROR *** Invalid mqbox %08X in %s\n",
            mqbox, __FUNCTION__);
        return false;
    }
    OSMessageQueue *dolQ = (OSMessageQueue*)mqbox;
    u32 dolFlags = lwpFlagsToDolFlags(flags);
    return OSSendMessage(dolQ, (OSMessage)msg, dolFlags);
}

BOOL MQ_Jam_dol(mqbox_t mqbox,mqmsg_t msg,u32 flags) {
    if(!PTR_VALID(mqbox)) {
        exiPrintf(" *** ERROR *** Invalid mqbox %08X in %s\n",
            mqbox, __FUNCTION__);
        return false;
    }
    OSMessageQueue *dolQ = (OSMessageQueue*)mqbox;
    u32 dolFlags = lwpFlagsToDolFlags(flags);
    //return OSJamMessage(dolQ, (OSMessage)msg, dolFlags);

    //OSJamMessage isn't in this game
    exiPrintf(" *** using OSSendMessage instead of OSJamMessage\n");
    return OSSendMessage(dolQ, (OSMessage)msg, dolFlags);
}

BOOL MQ_Receive_dol(mqbox_t mqbox,mqmsg_t *msg,u32 flags) {
    if(!PTR_VALID(mqbox)) {
        exiPrintf(" *** ERROR *** Invalid mqbox %08X in %s\n",
            mqbox, __FUNCTION__);
        return false;
    }
    OSMessageQueue *dolQ = (OSMessageQueue*)mqbox;
    u32 dolFlags = lwpFlagsToDolFlags(flags);
    return OSReceiveMessage(dolQ, (OSMessage*)msg, dolFlags);
}
