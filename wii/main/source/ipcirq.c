#include "main.h"

void _handleReply() {
    IpcRequestAndMsgQueue *req = NULL;

    req = (IpcRequestAndMsgQueue*)IPC_ReadReg(2);
    if(!req) {
        exiPuts(" *** ERROR *** Got NULL IPC reply\n");
        return;
    }
    req = MEM_PHYSICAL_TO_K0(req);
    //req points to the request that this reply is for.

    _ipcAckReply(); //reply early ACK
    DCInvalidateRange(req, sizeof(IpcRequestAndMsgQueue));
    IPC_DPRINT("IPC RESP: %08X\n", (u32)req);

    if(req->request.magic != IPC_REQ_MAGIC) {
        IpcRequest *r = &req->request;
        exiPrintf(" *** ERROR *** Received unknown IPC response (magic %08x):\n", r->magic);
        exiPrintf("  CMD %08x RES %08x REQCMD %08x\n", r->cmd, r->result, r->req_cmd);
        exiPrintf("  Args: %08x %08x %08x %08x %08x\n", r->args[0], r->args[1], r->args[2], r->args[3], r->args[4]);
        exiPrintf("  CB %08x DATA %08x REL %08x QUEUE %08x\n", (u32)r->cb, (u32)r->usrdata, r->relnch, (u32)r->syncqueue);
    }
    else {
        req->state = IPC_REQ_STATE_FINISHED;
        OSWakeupThread(&req->queue);
        IPC_DPRINT("IPC: sent resp to queue %08X->%08X\n",
            (u32)req, (u32)&req->queue);
    }
}

void _handleAck() {
    //acknowledge ACK (lol) - clear IY1, IY2; set Y2
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x02)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000);
	_ipcSendEndOfReply();
    _ipcWriteNextQueuedReq(); //keep the queue moving
}

void ipcPoll() {
    //get reply
    if((IPC_ReadReg(1) & 0x0014) == 0x0014) {
        _handleReply();
    }

    //get ACK
	if((IPC_ReadReg(1) & 0x0022) == 0x0022) {
        _handleAck();
    }

    ipcPumpQueue();
}

void ipcIrqHandler(int irqNo, OSContext *ctx) {
    //IPC (ACR) interrupt handler for communicating
    //with Wii IOS
    ipcPoll();
}
