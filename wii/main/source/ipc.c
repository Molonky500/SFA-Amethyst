#include "main.h"

u32 IPC_REQ_MAGIC;
u64 ipcTotalReqsSent = 0; //debug

//static u32 _ipc_seed = 0xffffffff;
OSMutex ipcMutex;

void initIpc() {
    // generate a random request magic
	//__ipc_srand(gettick());
	//IPC_REQ_MAGIC = __ipc_rand();
    IPC_REQ_MAGIC = 0xF0C57175;

    _ipc_bufferlo = _ipc_currbufferlo = __SYS_GetIPCBufferLo();
	_ipc_bufferhi = _ipc_currbufferhi = __SYS_GetIPCBufferHi();
    __IOS_InitHeap();
	memset(_ipcRequestQueue, 0, sizeof(_ipcRequestQueue));
	_ipcIdle = true;

    OSInitMutex(&ipcMutex);
    *(u32*)0x800030ac = (u32)ipcIrqHandler;
    __UnmaskIrq(IM_PI_ACR);
    IPC_WriteReg(1, 0x38); //HW_IPC_PPCCTRL
    IPC_DPRINT("IPC init OK, magic=0x%08X, buf=%08X-%08X\n",
        IPC_REQ_MAGIC, (u32)_ipc_bufferlo, (u32)_ipc_bufferhi);
	IPC_DPRINT("sizeof(IpcRequestAndMsgQueue) = %d; queue=%08X\n",
		sizeof(IpcRequestAndMsgQueue), (u32)_ipcRequestQueue);
}

void ipcDebugPrint() {
	/*debugPrintf("IPC buf %08X - %08X queue %d,%d %s\n",
		_ipc_currbufferlo, _ipc_currbufferhi,
		_ipcReqQueueHead, _ipcReqQueueTail,
		_ipcIdle ? "[idle]" : "");
	debugPrintf("Reqs: %08X %08X\n",
		(u32)(ipcTotalReqsSent >> 32),
		(u32)(ipcTotalReqsSent & 0xFFFFFFFFll));
	for(int i=0; i<IPC_NUMHEAPS; i++) {
		IpcHeap *heap = &_ipc_heaps[i];
		if(!heap->size) continue;
		heap_iblock info;
		__lwp_heap_getinfo(&heap->heap, &info);
		debugPrintf("heap %d: free %dblk %db used %dblk %db\n", i,
			info.free_blocks, info.free_size,
			info.used_size, info.used_blocks);
	}*/
}

s32 __ipc_syncrequest(IpcRequestAndMsgQueue *req) {
	s32 ret;

	IPC_DPRINT("IPC: sending sync req %08X\n",
		(u32)req);
	int irq = OSDisableInterrupts();
    //OSLockMutex(&ipcMutex);

	ret = _ipcWriteReq(req);
    if(ret) {
        //OSUnlockMutex(&ipcMutex);
		OSRestoreInterrupts(irq);
        printf(" *** ERROR *** _ipcWriteReq: %d\n", ret);
        return ret;
    }
    IPC_DPRINT("IPC: Wait for response: %08X\n", (u32)req);

    //now wait for interrupt for response
	OSRestoreInterrupts(irq);

	if(areInterruptsEnabled()) {
		OSSleepThread(&req->queue);
	}
	else {
		IPC_DPRINT("Polling for req %08X\n", (u32)req);
		while(req->state != IPC_REQ_STATE_FINISHED) {
			ipcPoll();
		}
	}
    IPC_DPRINT("IPC: Got response for req %08X\n", (u32)req);
    //NULL resp indicates only ACK reply
    //else resp is IpcRequest* response
	//resp is in a static buffer, no need to free

    ret = req->request.result;
	//OSUnlockMutex(&ipcMutex);
	//LWP_CloseQueue(req->syncqueue);
	return ret;
}

s32 __ipc_asyncrequest(IpcRequestAndMsgQueue *req) {
    IPC_DPRINT("IPC async req %08X\n", (u32)req);
    //OSLockMutex(&ipcMutex);
	int irq = OSDisableInterrupts();

    u32 ret = _ipcWriteReq(req);
    if(ret) {
        //OSUnlockMutex(&ipcMutex);
		OSRestoreInterrupts(irq);
        printf(" *** ERROR *** _ipcWriteReq: %d\n", ret);
        return ret;
    }
    IPC_DPRINT("Sent async IPC req: %08X\n", (u32)req);

    //OSUnlockMutex(&ipcMutex);
	OSRestoreInterrupts(irq);
    return 0;
}

s32 IOS_Open(const char *filepath,u32 mode) {
	s32 ret;
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;

	IPC_DPRINT("%s(%s, %08X) req=%08X\n", __FUNCTION__,
		filepath, mode, (u32)req);
	if(filepath==NULL) return -EINVAL;

	req->request.cmd = IOS_OPEN;
	req->request.cb = NULL;
	req->request.relnch = 0;

	DCFlushRange((void*)filepath,
        strnlen(filepath,IPC_MAXPATH_LEN) + 1);
	req->request.open.filepath = (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req->request.open.mode     = mode;

	IPC_DPRINT("%s: __ipc_syncrequest(%08X)...\n",
		__FUNCTION__, (u32)req);

	ret = __ipc_syncrequest(req);

	IPC_DPRINT("%s: __ipc_syncrequest(%08X) done\n",
		__FUNCTION__, (u32)req);

	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_OpenAsync(const char *filepath,u32 mode,ipccallback ipc_cb,
void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;

	IPC_DPRINT("%s(%s, %08X) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, filepath, mode, (u32)req,
		(u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_OPEN;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	DCFlushRange((void*)filepath,
		strnlen(filepath,IPC_MAXPATH_LEN) + 1);
	req->request.open.filepath	= (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req->request.open.mode		= mode;

	IPC_DPRINT("%s: __ipc_asyncrequest(%08X)\n",
		__FUNCTION__, (u32)req);
	return __ipc_asyncrequest(req);
}

s32 IOS_Close(s32 fd) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d) req=%08X\n", __FUNCTION__, fd, (u32)req);
	req->request.cmd = IOS_CLOSE;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;
	s32 ret = __ipc_syncrequest(req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_CloseAsync(s32 fd,ipccallback ipc_cb,void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d) req=%08X cb=%08X(%08X)\n", __FUNCTION__,
		fd, (u32)req, (u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_CLOSE;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	return __ipc_asyncrequest(req);
}

s32 IOS_Read(s32 fd,void *buf,s32 len) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X\n", __FUNCTION__,
		fd, (u32)buf, len, (u32)req);
	req->request.cmd = IOS_READ;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;

	DCInvalidateRange(buf,len);
	req->request.read.data = (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->request.read.len  = len;

	IPC_DPRINT("%s: send %08X\n", __FUNCTION__, (u32)req);
	s32 ret = __ipc_syncrequest(req);
	IPC_DPRINT("%s: done %08X\n", __FUNCTION__, (u32)req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_ReadAsync(s32 fd,void *buf,s32 len,ipccallback ipc_cb,
void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, fd, (u32)buf, len, (u32)req,
		(u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_READ;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	DCInvalidateRange(buf,len);
	req->request.read.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->request.read.len	= len;

	return __ipc_asyncrequest(req);
}

s32 IOS_Write(s32 fd,const void *buf,s32 len) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X\n", __FUNCTION__,
		fd, (u32)buf, len, (u32)req);
	req->request.cmd = IOS_WRITE;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;

	DCFlushRange((void*)buf,len);
	req->request.write.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->request.write.len	= len;

	s32 ret = __ipc_syncrequest(req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_WriteAsync(s32 fd,const void *buf,s32 len,
ipccallback ipc_cb,void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, fd, (u32)buf, len, (u32)req,
		(u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_WRITE;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	DCFlushRange((void*)buf,len);
	req->request.write.data		= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->request.write.len		= len;

	return __ipc_asyncrequest(req);
}

s32 IOS_Seek(s32 fd,s32 where,s32 whence) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X\n", __FUNCTION__,
		fd, where, whence, (u32)req);
	req->request.cmd = IOS_SEEK;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;
	req->request.seek.where  = where;
	req->request.seek.whence = whence;

	s32 ret = __ipc_syncrequest(req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_SeekAsync(s32 fd,s32 where,s32 whence,
ipccallback ipc_cb,void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %08X, %d) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, fd, where, whence, (u32)req,
		(u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_SEEK;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;
	req->request.seek.where	 = where;
	req->request.seek.whence = whence;

	return __ipc_asyncrequest(req);
}

s32 IOS_Ioctl(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %d, %08X, %d, %08X, %d) req=%08X\n",
		__FUNCTION__, fd, ioctl, (u32)buffer_in, len_in,
		(u32)buffer_io, len_io, (u32)req);
	req->request.cmd = IOS_IOCTL;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;
	req->request.ioctl.ioctl     = ioctl;
	req->request.ioctl.buffer_in = (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_in);
	req->request.ioctl.len_in    = len_in;
	req->request.ioctl.buffer_io = (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_io);
	req->request.ioctl.len_io    = len_io;
	DCFlushRange(buffer_in,len_in);
	DCFlushRange(buffer_io,len_io);

	s32 ret = __ipc_syncrequest(req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_IoctlAsync(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,
void *buffer_io,s32 len_io,ipccallback ipc_cb,void *usrdata) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %d, %08X, %d, %08X, %d) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, fd, ioctl, (u32)buffer_in, len_in,
		(u32)buffer_io, len_io, (u32)req, (u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_IOCTL;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	req->request.ioctl.ioctl		= ioctl;
	req->request.ioctl.buffer_in	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_in);
	req->request.ioctl.len_in		= len_in;
	req->request.ioctl.buffer_io	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_io);
	req->request.ioctl.len_io		= len_io;

	DCFlushRange(buffer_in,len_in);
	DCFlushRange(buffer_io,len_io);

	return __ipc_asyncrequest(req);
}

s32 IOS_Ioctlv(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv) {
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %d, %d, %d, %08X) req=%08X\n", __FUNCTION__,
		fd, ioctl, cnt_in, cnt_io, (u32)argv, (u32)req);
	req->request.cmd = IOS_IOCTLV;
	req->request.fd = fd;
	req->request.cb = NULL;
	req->request.relnch = 0;
	req->request.ioctlv.ioctl  = ioctl;
	req->request.ioctlv.argcin = cnt_in;
	req->request.ioctlv.argcio = cnt_io;
	req->request.ioctlv.argv   = (struct _ioctlv*)MEM_VIRTUAL_TO_PHYSICAL(argv);

	for(int i=0; i<cnt_in; i++) {
		if(argv[i].data != NULL && argv[i].len > 0) {
			DCFlushRange(argv[i].data, argv[i].len);
			argv[i].data = (void*)MEM_VIRTUAL_TO_PHYSICAL(argv[i].data);
		}
	}

	for(int i=0; i<cnt_io; i++) {
		if(argv[cnt_in+i].data != NULL && argv[cnt_in+i].len > 0) {
			DCFlushRange(argv[cnt_in+i].data, argv[cnt_in+i].len);
			argv[cnt_in+i].data = (void*)MEM_VIRTUAL_TO_PHYSICAL(argv[cnt_in+i].data);
		}
	}
	DCFlushRange(argv,((cnt_in+cnt_io)<<3));

	s32 ret = __ipc_syncrequest(req);
	if(req) __ipc_freereq(req);
	return ret;
}

s32 IOS_IoctlvAsync(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,
ioctlv *argv,ipccallback ipc_cb,void *usrdata) {
	s32 i;
	IpcRequestAndMsgQueue *req = __ipc_allocreq();
	if(!req) return IPC_ENOMEM;
	IPC_DPRINT("%s(%d, %d, %d, %d, %08X) req=%08X cb=%08X(%08X)\n",
		__FUNCTION__, fd, ioctl, cnt_in, cnt_io, (u32)argv,
		(u32)req, (u32)ipc_cb, (u32)usrdata);

	req->request.cmd = IOS_IOCTLV;
	req->request.fd = fd;
	req->request.cb = ipc_cb;
	req->request.usrdata = usrdata;
	req->request.relnch = 0;

	req->request.ioctlv.ioctl	= ioctl;
	req->request.ioctlv.argcin	= cnt_in;
	req->request.ioctlv.argcio	= cnt_io;
	req->request.ioctlv.argv	= (struct _ioctlv*)MEM_VIRTUAL_TO_PHYSICAL(argv);

	i = 0;
	while(i<cnt_in) {
		if(argv[i].data!=NULL && argv[i].len>0) {
			DCFlushRange(argv[i].data,argv[i].len);
			argv[i].data = (void*)MEM_VIRTUAL_TO_PHYSICAL(argv[i].data);
		}
		i++;
	}

	i = 0;
	while(i<cnt_io) {
		if(argv[cnt_in+i].data!=NULL && argv[cnt_in+i].len>0) {
			DCFlushRange(argv[cnt_in+i].data,argv[cnt_in+i].len);
			argv[cnt_in+i].data = (void*)MEM_VIRTUAL_TO_PHYSICAL(argv[cnt_in+i].data);
		}
		i++;
	}
	DCFlushRange(argv,((cnt_in+cnt_io)<<3));

	return __ipc_asyncrequest(req);
}
