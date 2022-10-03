#include "main.h"

#define IOS_OPEN				0x01
#define IOS_CLOSE				0x02
#define IOS_READ				0x03
#define IOS_WRITE				0x04
#define IOS_SEEK				0x05
#define IOS_IOCTL				0x06
#define IOS_IOCTLV				0x07

static u32 IPC_REQ_MAGIC;

static u32 _ipc_seed = 0xffffffff;
static OSMutex ipcMutex;
static vu32 nAck=0; //okay if it overflows

typedef struct {
    OSMessageQueue queue;
    OSMessage msgs[0]; //at least one
} _AsyncMsgQueue;

_AsyncMsgQueue* allocAsyncMsgQueue(int nMsgs) {
    return malloc(sizeof(_AsyncMsgQueue) +
        (sizeof(OSMessage) * nMsgs));
}

static __inline__ u32 IPC_ReadReg(u32 reg) {
	return _ipcReg[reg];
}

static __inline__ void IPC_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg] = val;
}

static __inline__ void ACR_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg>>2] = val;
}

static __inline__ void ackReply() {
    //clear IY1, IY2 (interrupt flags); set Y1
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x04)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000); //XXX why? we're not even
        //supposed to be able to access this.
}

static __inline__ void ackAck() {
    //clear IY1, IY2; set Y2
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x02)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000);
}

static __inline__ void sendEndOfReply() {
    //clear IY1, IY2; clear X2 (by writing 1)
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x08));
}


void initIpc() {
    // generate a random request magic
	//__ipc_srand(gettick());
	//IPC_REQ_MAGIC = __ipc_rand();
    IPC_REQ_MAGIC = 0xF0C57175;

    OSInitMutex(&ipcMutex);
    *(u32*)0x800030ac = (u32)ipcIrqHandler;
    __UnmaskIrq(IM_PI_ACR);
    IPC_WriteReg(1, 0x38); //HW_IPC_PPCCTRL
    IPC_DPRINT("IPC init OK, magic=0x%08X\n", IPC_REQ_MAGIC);
}


static int _writeReq(IpcRequest *req) {
    req->magic = IPC_REQ_MAGIC;
    IPC_DPRINT("IPC send req %08X\n", (u32)req);
    DCFlushRange(req, sizeof(IpcRequest));
    IPC_WriteReg(0, MEM_VIRTUAL_TO_PHYSICAL(req)); //HW_IPC_PPCMSG
    IPC_WriteReg(1, (IPC_ReadReg(1)&0x30)|0x01); //HW_IPC_PPCCTRL
    return 0;
}

s32 __ipc_syncrequest(IpcRequest *req) {
	s32 ret;
    OSMessage msg;
    OSMessageQueue queue;

    OSLockMutex(&ipcMutex);
    req->syncqueue = &queue;
    req->bFreeWhenDone = 0;
	OSInitMessageQueue(&queue, &msg, 1);

    u32 prevAck = (u32)nAck;
	ret = _writeReq(req);
    if(ret) {
        OSUnlockMutex(&ipcMutex);
        printf(" *** ERROR *** _writeReq: %d\n", ret);
        return ret;
    }
    //wait for ack
    while(prevAck == nAck);

    //now wait for interrupt for response
    OSMessage resp;
    if(!OSReceiveMessage(&queue, &resp, OS_MESSAGE_BLOCK)) {
        OSUnlockMutex(&ipcMutex);
        printf(" *** ERROR *** %s:OSReceiveMessage failed\n",
            __FUNCTION__);
        return -EPIPE;
    }
    //NULL resp indicates only ACK reply
    //else resp is IpcRequest* response

    ret = req->result;
	OSUnlockMutex(&ipcMutex);
	//LWP_CloseQueue(req->syncqueue);
	return ret;
}

s32 __ipc_asyncrequest(IpcRequest *req) {
    OSLockMutex(&ipcMutex);

    req->bFreeWhenDone = 1;
    _AsyncMsgQueue *queue = allocAsyncMsgQueue(1);
    if(!queue) {
        //XXX free(req) ?
        OSUnlockMutex(&ipcMutex);
        return -ENOMEM;
    }
    req->syncqueue = &queue->queue;
    OSInitMessageQueue(&queue->queue, queue->msgs, 1);

    u32 prevAck = (u32)nAck;
	u32 ret = _writeReq(req);
    if(ret) {
        free(req->syncqueue);
        req->syncqueue = NULL;
        OSUnlockMutex(&ipcMutex);
        printf(" *** ERROR *** _writeReq: %d\n", ret);
        return ret;
    }
    //wait for ack
    while(prevAck == nAck);

    OSUnlockMutex(&ipcMutex);
    return 0;
}

static void __ipc_replyhandler(bool isAck) {
    IpcRequest *req = NULL;

    if(!isAck) {
        req = (IpcRequest*)IPC_ReadReg(2);
        if(!req) return;
        //req points to the request that this reply is for.
        ackReply(); //reply early ACK
    }
    else ackAck();

    IPC_DPRINT("IPC %s: %08X\n", isAck ? "ACK" : "RESP", req);
    if(!isAck) {
        req = MEM_PHYSICAL_TO_K0(req);
        DCInvalidateRange(req, sizeof(IpcRequest));

        if(req->magic != IPC_REQ_MAGIC) {
            printf(" *** ERROR *** Received unknown IPC response (magic %08x):\n", req->magic);
            printf("  CMD %08x RES %08x REQCMD %08x\n", req->cmd, req->result, req->req_cmd);
            printf("  Args: %08x %08x %08x %08x %08x\n", req->args[0], req->args[1], req->args[2], req->args[3], req->args[4]);
            printf("  CB %08x DATA %08x REL %08x QUEUE %08x\n", (u32)req->cb, (u32)req->usrdata, req->relnch, (u32)req->syncqueue);
        }
        else if(!OSSendMessage(req->syncqueue, (OSMessage)req, OS_MESSAGE_NOBLOCK)) {
            printf(" *** ERROR *** IPC reply dropped (queue %08X full)\n",
                (u32)req->syncqueue);
        }
        if(req->bFreeWhenDone) {
            free(req->syncqueue);
            free(req);
        }
    }
    else {
        nAck++;
    }

    sendEndOfReply();
}

void ipcIrqHandler(int irqNo, OSContext *ctx) {
    //IPC (ACR) interrupt handler for communicating
    //with Wii IOS

    //get reply
    if((IPC_ReadReg(1) & 0x0014) == 0x0014) {
        __ipc_replyhandler(false);
    }

    //get ACK
	if((IPC_ReadReg(1) & 0x0022) == 0x0022) {
        __ipc_replyhandler(true);
    }
}
