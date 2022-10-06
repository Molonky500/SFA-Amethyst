#include "main.h"
//handle request queue and such

__attribute__ ((aligned (32)))
IpcRequestAndMsgQueue _ipcRequestQueue[IPC_QUEUE_MAX];
vs32 _ipcReqQueueHead=0, _ipcReqQueueTail=0;
vs32 _prevHead=0, _prevTail=0;
bool _ipcIdle = true; //other end is waiting for next request

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
    if(!req->ready) {
        IPC_DPRINT("IPC: idle\n");
        _ipcIdle = true;
        OSRestoreInterrupts(irq);
        return;
    }
    IPC_DPRINT("IPC: next queued req %08X (head=%d tail=%d)\n",
        (u32)req, _ipcReqQueueHead, next);

    if(next <= _prevTail && next > 1) {
        exiPrintf(" *** ERROR *** IPC tail corrupted (%d -> %d)\n",
            _prevTail, next);
    }

    if(req->request.magic != IPC_REQ_MAGIC) {
        exiPrintf(" *** ERROR *** Invalid queued IPC req %08X (magic=%08X)\n",
            (u32)req, req->request.magic);
        OSRestoreInterrupts(irq);
        return;
    }
    _prevTail = _ipcReqQueueTail;
    _ipcReqQueueTail = next;
	_ipcIdle = false;
    ipcTotalReqsSent++;
	DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));
    IPC_WriteReg(0, MEM_VIRTUAL_TO_PHYSICAL(req)); //HW_IPC_PPCMSG
    IPC_WriteReg(1, (IPC_ReadReg(1)&0x30)|0x01); //HW_IPC_PPCCTRL
    OSRestoreInterrupts(irq);
}

int _ipcWriteReq(IpcRequestAndMsgQueue *req) {
	int irq = OSDisableInterrupts();
    req->request.magic = IPC_REQ_MAGIC;
    req->ready = true;
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
