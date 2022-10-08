#include "main.h"

void _ipcHandleReply(IpcRequestAndMsgQueue *req) {
    if(!req) {
        exiPuts(" *** ERROR *** Got NULL IPC reply\n");
        return;
    }
    req = MEM_PHYSICAL_TO_VIRTUAL(req);
    //req points to the request that this reply is for.

    IPC_DPRINT("IPC RESP: %08X (ID %d cmd 0x%x state %d)\n",
        (u32)req, req->id, req->request.cmd, req->state);
    DCInvalidateRange(req, sizeof(IpcRequestAndMsgQueue));

    if(req->request.magic != IPC_REQ_MAGIC) {
        IpcRequest *r = &req->request;
        exiPrintf(" *** ERROR *** Received unknown IPC response (magic %08x):\n", r->magic);
        exiPrintf("  CMD %08x RES %08x REQCMD %08x\n", r->cmd, r->result, r->req_cmd);
        exiPrintf("  Args: %08x %08x %08x %08x %08x\n", r->args[0], r->args[1], r->args[2], r->args[3], r->args[4]);
        exiPrintf("  CB %08x DATA %08x REL %08x QUEUE %08x\n", (u32)r->cb, (u32)r->usrdata, r->relnch, (u32)r->syncqueue);
        HALT;
    }
    else {
        if(req->state != IPC_REQ_STATE_PROCESSING) {
            IPC_DPRINT(" *** IPC got resp for request %d in state %d\n",
                req->id, req->state);
        }
        req->state = IPC_REQ_STATE_FINISHING;
        DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));
    }
}

void _handleAck() {
    IpcRequestAndMsgQueue *req;

    int nAck = 0;
    int next = (_ipcReqSlotWait+1) % IPC_QUEUE_MAX;
    for(int i=0; i<IPC_QUEUE_MAX; i++) {
        req = &_ipcRequestQueue[(_ipcReqSlotWait+i) % IPC_QUEUE_MAX];
        if(req->state == IPC_REQ_STATE_PENDING) {
            if(nAck > 0) {
                exiPrintf(" *** ERROR *** IPC: multiple requests in PENDING state\n");
                ipcDumpQueueForDebug();
                HALT;
            }
            IPC_DPRINT("Got ACK for req %d\n", req->id);
            req->state = IPC_REQ_STATE_PROCESSING;
            DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));
            nAck++;
        }
    }
    if(!nAck) {
        exiPuts(" *** ERROR *** IPC: got ACK but nothing pending\n");
    }
    _ipcReqSlotWait = next;
    //acknowledge ACK (lol) - write 1 to clear Y2 (ACK)
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x02)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000);
	_ipcSendEndOfReply();
    //_ipcWriteNextQueuedReq(); //keep the queue moving
}

void ipcPoll() {
    int irq = OSDisableInterrupts();

    //X1     Read/write flag X1
    //Y2     Read flag Y2. To clear the flag, write 1 to this bit.
    //Y1     Read flag Y1. To clear the flag, write 1 to this bit.
    //X2     Read/write flag X2
    //IY1    If set, flag Y1 generates Hollywood interrupt #30
    //IY2    If set, flag Y2 generates Hollywood interrupt #30
    //Field  Description   [ IOS flag protocol ]
    //01 X1  Execute command: a new pointer is available in HW_IPC_PPCCTRL
    //02 Y2  Command acknowledge
    //04 Y1  Command executed and reply available in HW_IPC_ARMMSG
    //08 X2  Relaunch
    //10 IY1
    //20 IY2

    //get reply
    if((IPC_ReadReg(1) & 0x0014) == 0x0014) {
        IpcRequestAndMsgQueue *req =
            (IpcRequestAndMsgQueue*)IPC_ReadReg(2);
        _ipcAckReply(); //reply early ACK
        _ipcHandleReply(req);
    }

    //get ACK
	if((IPC_ReadReg(1) & 0x0022) == 0x0022) {
        _handleAck();
    }

    ipcPumpQueue();
    OSRestoreInterrupts(irq);
}

void ipcIrqHandler(int irqNo, OSContext *ctx) {
    //IPC (ACR) interrupt handler for communicating
    //with Wii IOS
    ipcPoll();
}
