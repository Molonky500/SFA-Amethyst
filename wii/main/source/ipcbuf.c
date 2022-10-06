#include "main.h"
//handle request queue and such

__attribute__ ((aligned (32)))
IpcRequestAndMsgQueue _ipcRequestQueue[IPC_QUEUE_MAX];
vs32 _ipcReqQueueHead=0; //next slot to allocate
vs32 _ipcReqQueueTail=0; //next slot to send to IOS
s32  _ipcReqNext=0; //next slot to check for completion
bool _ipcIdle = true; //other end is waiting for next request

void ipcPumpQueue() {
    while(_ipcReqNext != _ipcReqQueueTail) {
        IpcRequestAndMsgQueue *req = &_ipcRequestQueue[_ipcReqNext];
        if(req->state != IPC_REQ_STATE_FINISHED) break;
        IPC_DPRINT("IPC: Finished %08X\n", (u32)req);
        if(req->request.cb) {
            IPC_DPRINT("IPC: cb %08X(%08X, %08X) req %08X\n",
                (u32)req->request.cb, (u32)req->request.result,
                (u32)req->request.usrdata, (u32)req);
            req->request.cb(req->request.result, req->request.usrdata);
            __ipc_freereq(req);
        }
        //if no cb, this is a synchronous request, so do not
        //free it; that's the responsibility of the caller.
        _ipcReqNext = (_ipcReqNext+1) % IPC_QUEUE_MAX;
    }
    _ipcWriteNextQueuedReq();
}

void _ipcWriteNextQueuedReq() {
    //this may be called from an IRQ handler
    //so a mutex won't work here
    int irq = OSDisableInterrupts();
	if(_ipcReqQueueTail == _ipcReqQueueHead) {
        IPC_DPRINT("IPC: idle\n");
		_ipcIdle = true;
        OSRestoreInterrupts(irq);
		return;
	}
	int next = (_ipcReqQueueTail + 1) % IPC_QUEUE_MAX;
	IpcRequestAndMsgQueue *req =
        &_ipcRequestQueue[_ipcReqQueueTail];
    if(req->state != IPC_REQ_STATE_QUEUED) {
        IPC_DPRINT("IPC: idle (next req state %d)\n", req->state);
        _ipcIdle = true;
        OSRestoreInterrupts(irq);
        return;
    }
    IPC_DPRINT("IPC: next queued req %08X (head=%d tail=%d)\n",
        (u32)req, _ipcReqQueueHead, next);

    if(req->request.magic != IPC_REQ_MAGIC) {
        exiPrintf(" *** ERROR *** Invalid queued IPC req %08X (magic=%08X)\n",
            (u32)req, req->request.magic);
        OSRestoreInterrupts(irq);
        return;
    }
    _ipcReqQueueTail = next;
	_ipcIdle = false;
    ipcTotalReqsSent++;
    req->state = IPC_REQ_STATE_PENDING;
	DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));
    IPC_WriteReg(0, MEM_VIRTUAL_TO_PHYSICAL(req)); //HW_IPC_PPCMSG
    IPC_WriteReg(1, (IPC_ReadReg(1)&0x30)|0x01); //HW_IPC_PPCCTRL
    OSRestoreInterrupts(irq);
}

int _ipcWriteReq(IpcRequestAndMsgQueue *req) {
	int irq = OSDisableInterrupts();
    req->request.magic = IPC_REQ_MAGIC;
    req->state = IPC_REQ_STATE_QUEUED;
	DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));

    if(req < &_ipcRequestQueue[0]
    || req >= &_ipcRequestQueue[IPC_QUEUE_MAX]) {
        exiPrintf(" *** ERROR *** pushing invalid IPC req %08x at %08x\n",
            (u32)req, RETURN_ADDRESS);
        OSRestoreInterrupts(irq);
        return -EINVAL;
    }

    //if not waiting on any ACK, send the next queued request
    //now, so that the queue starts moving.
    //else the ACK will trigger sending the next queued request.
	if(_ipcIdle) _ipcWriteNextQueuedReq();
	OSRestoreInterrupts(irq);

    return 0;
}
