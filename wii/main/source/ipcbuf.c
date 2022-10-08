#include "main.h"
//handle request queue and such

__attribute__ ((aligned (32)))
IpcRequestAndMsgQueue _ipcRequestQueue[IPC_QUEUE_MAX];
vs32 _ipcReqQueueHead=0; //next slot to allocate
vs32 _ipcReqQueueTail=0; //next slot to send to IOS
vs32 _ipcReqSlotCur  =0; //slot currently awaiting completion
vs32 _ipcReqSlotWait =0; //slot currently awaiting ACK
bool _ipcIdle = true; //other end is waiting for next request

void ipcDumpQueueForDebug() {
    static const char *stateNames[] = {
        "EMPTY    ",
        "PREPARING",
        "QUEUED   ",
        "PENDING  ",
        "PROCESSNG", //sic to fit
        "FINISHING",
        "FINISHED "
    };
    static const char *cmdNames[] = {
        "NULL  ",
        "OPEN  ",
        "CLOSE ",
        "READ  ",
        "WRITE ",
        "SEEK  ",
        "IOCTL ",
        "IOCTLV",
        "ASYNC ", //messages that are automatically sent to an IOS queue when an asynchronous syscall completes
        "NEWMSG" //used internally by IOSP to indicate that a new unprocessed message has arrived.
    };
    int irq = OSDisableInterrupts();
    exiPrintf("    Idx   State        Cmd      fd#     ReqID ReqAddr  Callback ThrdHead ThrdTail\n");
    int idx = _ipcReqQueueTail;
    for(int i=0; i<IPC_QUEUE_MAX; i++) {
        IpcRequestAndMsgQueue *req = &_ipcRequestQueue[idx];
        exiPrintf("%c%c%c%c%4d %2X %s %2X %s %3d %9d %8X %8X %8X %8X\n",
            (idx == _ipcReqQueueTail) ? 'T' :' ',
            (idx == _ipcReqQueueHead) ? 'H' :' ',
            (idx == _ipcReqSlotCur)   ? 'N' :' ',
            (idx == _ipcReqSlotWait)  ? 'W' :' ',
            idx, req->state,
            req->state < IPC_NUM_REQ_STATES ? stateNames[req->state] : "?????????",
            req->request.cmd,
            req->request.cmd < 10 ? cmdNames[req->request.cmd] : "??????",
            req->request.fd, req->id, (u32)req, (u32)req->request.cb,
            (u32)req->queue.head, (u32)req->queue.tail);
        idx = (idx+1) % IPC_QUEUE_MAX;
    }
    OSRestoreInterrupts(irq);
}

void ipcPumpQueue() {
    int irq = OSDisableInterrupts();
    for(int i=0; i<IPC_QUEUE_MAX; i++) {
        IpcRequestAndMsgQueue *req = &_ipcRequestQueue[i];

        //if this request is waiting for PPC to deal with
        //the response, deal with it now.
        if(req->state == IPC_REQ_STATE_FINISHING) {
            IPC_DPRINT("IPC: Finishing %d\n", req->id);
            if(req->request.cb) {
                IPC_DPRINT("IPC: cb %08X(%08X, %08X) req %d\n",
                    (u32)req->request.cb, (u32)req->request.result,
                    (u32)req->request.usrdata, req->id);
                req->request.cb(req->request.result, req->request.usrdata);
                __ipc_freereq(req); //sets state to EMPTY
            }
            else {
                req->state = IPC_REQ_STATE_FINISHED;
                OSWakeupThread(&req->queue);
                IPC_DPRINT("IPC: sent resp to queue %d->%08X\n",
                    req->id, (u32)&req->queue);
                //if no cb, this is a synchronous request, so
                //do not free it; that's the responsibility
                //of the caller.
            }
        }
    }

    /*while(_ipcReqSlotCur != _ipcReqQueueTail) {
        IpcRequestAndMsgQueue *req = &_ipcRequestQueue[_ipcReqSlotCur];
        if(req->state != IPC_REQ_STATE_FINISHING) break;

        _ipcReqSlotCur = (_ipcReqSlotCur+1) % IPC_QUEUE_MAX;
    }*/
    _ipcWriteNextQueuedReq();
    OSRestoreInterrupts(irq);
}

void _ipcWriteNextQueuedReq() {
    IpcRequestAndMsgQueue *req;

    //this may be called from an IRQ handler
    //so a mutex won't work here
    int irq = OSDisableInterrupts();
	/*if(_ipcReqQueueTail == _ipcReqQueueHead) {
        IPC_DPRINT("IPC: idle\n");
		_ipcIdle = true;
        OSRestoreInterrupts(irq);
		return;
	}*/

    //do we have an async reply?
    /*for(int i=0; i<IPC_QUEUE_MAX; i++) {
        req = &_ipcRequestQueue[i];
        if(req->state == IPC_REQ_STATE_PROCESSING
        && req->request.cmd == 8) {
            IPC_DPRINT("IPC: got async reply for %d (%08X)\n",
                req->id, (u32)req);
            _ipcHandleReply(req);
        }
    }*/

    //has the most recently-sent request been received?
    for(int i=0; i<IPC_QUEUE_MAX; i++) {
        req = &_ipcRequestQueue[i];
        if(req->state == IPC_REQ_STATE_PENDING) {
            IPC_DPRINT("IPC: idle (cur req state %d)\n", req->state);
            _ipcIdle = true;
            OSRestoreInterrupts(irq);
            return;
        }
    }

    //is a request ready to be sent?
    int next = (_ipcReqQueueTail + 1) % IPC_QUEUE_MAX;
    for(int i=0; i<IPC_QUEUE_MAX; i++) {
        req = &_ipcRequestQueue[(_ipcReqQueueTail + i) % IPC_QUEUE_MAX];
        if(req->state == IPC_REQ_STATE_QUEUED) break;
        req = NULL;
        next = (next+1) % IPC_QUEUE_MAX;
    }
    if(!req) {
        //exiPrintf("IPC: no queued requests\n");
        OSRestoreInterrupts(irq);
        return;
    }
    IPC_DPRINT("IPC: send next queued req %d (head=%d tail=%d)\n",
        req->id, _ipcReqQueueHead, next);

    //is the request valid?
    if(req->request.magic != IPC_REQ_MAGIC) {
        exiPrintf(" *** ERROR *** Invalid queued IPC req %d (magic=%08X)\n",
            req->id, req->request.magic);
        OSRestoreInterrupts(irq);
        return;
    }
    _ipcReqQueueTail = next;
	_ipcIdle = false;
    ipcTotalReqsSent++;
    int tryCnt=0;
    req->state = IPC_REQ_STATE_PENDING;
	DCFlushRange(req, sizeof(IpcRequestAndMsgQueue));
    IPC_WriteReg(0, MEM_VIRTUAL_TO_PHYSICAL(req)); //HW_IPC_PPCMSG
    //set X1 to tell IOS that a command is ready
    IPC_WriteReg(1, (IPC_ReadReg(1)&0x30)|0x01); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000);

    /*while((IPC_ReadReg(1) & 0x0022) != 0x0022) { //wait for ACK
        if(++tryCnt >= 10000000) {
            exiPrintf(" *** ERROR *** IPC: no ACK for %08X (ID %d); HW_IPC_PPCCTRL=%08X HW_IPC_PPCMSG=%08X\n",
                (u32)req, req->id, IPC_ReadReg(0), IPC_ReadReg(1));
            HALT;
        }
    }
    req->state = IPC_REQ_STATE_PROCESSING;
    //acknowledge ACK (lol) - clear Y2 (ACK flag w1c)
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x02)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000);
	_ipcSendEndOfReply();
    exiPuts("GOT ACK\n");*/
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
	/*if(_ipcIdle)*/ _ipcWriteNextQueuedReq();
	OSRestoreInterrupts(irq);

    return 0;
}
