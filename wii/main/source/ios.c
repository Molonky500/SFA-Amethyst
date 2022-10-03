#include "main.h"

static void *_ipc_bufferlo = NULL;
static void *_ipc_bufferhi = NULL;
static void *_ipc_currbufferlo = NULL;
static void *_ipc_currbufferhi = NULL;

void* IPC_GetBufferLo(void) {
	return _ipc_currbufferlo;
}

void* IPC_GetBufferHi(void) {
	return _ipc_currbufferhi;
}

void IPC_SetBufferLo(void *bufferlo) {
	if(_ipc_bufferlo<=bufferlo) _ipc_currbufferlo = bufferlo;
}

void IPC_SetBufferHi(void *bufferhi) {
	if(bufferhi<=_ipc_bufferhi) _ipc_currbufferhi = bufferhi;
}

s32 IOS_Open(const char *filepath,u32 mode) {
	s32 ret;
	IpcRequest req;

	IPC_DPRINT("%s(%s, %08X)\n", __FUNCTION__, filepath, mode);
	if(filepath==NULL) return -EINVAL;

	req.cmd = IOS_OPEN;
	req.cb = NULL;
	req.relnch = 0;

	DCFlushRange((void*)filepath,
        strnlen(filepath,IPC_MAXPATH_LEN) + 1);

	req.open.filepath = (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req.open.mode     = mode;
	IPC_DPRINT("__ipc_syncrequest...\n");
	ret = __ipc_syncrequest(&req);
	IPC_DPRINT("__ipc_syncrequest done\n");
	return ret;
}

s32 IOS_OpenAsync(const char *filepath,u32 mode,ipccallback ipc_cb,
void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_OPEN;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCFlushRange((void*)filepath,strnlen(filepath,IPC_MAXPATH_LEN) + 1);

	req->open.filepath	= (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req->open.mode		= mode;

	return __ipc_asyncrequest(req);
}

s32 IOS_Close(s32 fd) {
	IPC_DPRINT("%s(%d)\n", __FUNCTION__, fd);
	IpcRequest req;
	req.cmd = IOS_CLOSE;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;
	return __ipc_syncrequest(&req);
}

s32 IOS_CloseAsync(s32 fd,ipccallback ipc_cb,void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_CLOSE;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	return __ipc_asyncrequest(req);
}

s32 IOS_Read(s32 fd,void *buf,s32 len) {
	IPC_DPRINT("%s(%d, %08X, %d)\n", __FUNCTION__, fd, (u32)buf, len);
	IpcRequest req;
	req.cmd = IOS_READ;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;

	DCInvalidateRange(buf,len);
	req.read.data = (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req.read.len  = len;

	return __ipc_syncrequest(&req);
}

s32 IOS_ReadAsync(s32 fd,void *buf,s32 len,ipccallback ipc_cb,
void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_READ;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCInvalidateRange(buf,len);
	req->read.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->read.len	= len;

	return __ipc_asyncrequest(req);
}

s32 IOS_Write(s32 fd,const void *buf,s32 len) {
	IPC_DPRINT("%s(%d, %08X, %d)\n", __FUNCTION__, fd, (u32)buf, len);
	IpcRequest req;
	req.cmd = IOS_WRITE;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;

	DCFlushRange((void*)buf,len);
	req.write.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req.write.len	= len;
	return __ipc_syncrequest(&req);
}

s32 IOS_WriteAsync(s32 fd,const void *buf,s32 len,
ipccallback ipc_cb,void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_WRITE;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCFlushRange((void*)buf,len);
	req->write.data		= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->write.len		= len;

	return __ipc_asyncrequest(req);
}

s32 IOS_Seek(s32 fd,s32 where,s32 whence) {
	IPC_DPRINT("%s(%d, %08X, %d)\n", __FUNCTION__, fd, where, whence);
	IpcRequest req;
	req.cmd = IOS_SEEK;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;
	req.seek.where  = where;
	req.seek.whence = whence;
	return __ipc_syncrequest(&req);
}

s32 IOS_SeekAsync(s32 fd,s32 where,s32 whence,
ipccallback ipc_cb,void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_SEEK;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	req->seek.where		= where;
	req->seek.whence	= whence;

	return __ipc_asyncrequest(req);
}

s32 IOS_Ioctl(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io) {
	IPC_DPRINT("%s(%d, %d, %08X, %d, %08X, %d)\n", __FUNCTION__,
		fd, ioctl, (u32)buffer_in, len_in, (u32)buffer_io, len_io);
	IpcRequest req;
	req.cmd = IOS_IOCTL;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;
	req.ioctl.ioctl     = ioctl;
	req.ioctl.buffer_in = (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_in);
	req.ioctl.len_in    = len_in;
	req.ioctl.buffer_io = (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_io);
	req.ioctl.len_io    = len_io;
	DCFlushRange(buffer_in,len_in);
	DCFlushRange(buffer_io,len_io);
	return __ipc_syncrequest(&req);
}

s32 IOS_IoctlAsync(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,
void *buffer_io,s32 len_io,ipccallback ipc_cb,void *usrdata) {
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(req==NULL) return IPC_ENOMEM;

	req->cmd = IOS_IOCTL;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	req->ioctl.ioctl		= ioctl;
	req->ioctl.buffer_in	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_in);
	req->ioctl.len_in		= len_in;
	req->ioctl.buffer_io	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_io);
	req->ioctl.len_io		= len_io;

	DCFlushRange(buffer_in,len_in);
	DCFlushRange(buffer_io,len_io);

	return __ipc_asyncrequest(req);
}

s32 IOS_Ioctlv(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv) {
	IPC_DPRINT("%s(%d, %d, %d, %d, %08X)\n", __FUNCTION__,
		fd, ioctl, cnt_in, cnt_io, (u32)argv);
	IpcRequest req;
	req.cmd = IOS_IOCTLV;
	req.fd = fd;
	req.cb = NULL;
	req.relnch = 0;
	req.ioctlv.ioctl  = ioctl;
	req.ioctlv.argcin = cnt_in;
	req.ioctlv.argcio = cnt_io;
	req.ioctlv.argv   = (struct _ioctlv*)MEM_VIRTUAL_TO_PHYSICAL(argv);

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
	return __ipc_syncrequest(&req);
}

s32 IOS_IoctlvAsync(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,
ioctlv *argv,ipccallback ipc_cb,void *usrdata) {
	s32 i;
	IpcRequest *req = malloc(sizeof(IpcRequest));
	if(!req) return -ENOMEM;

	req->cmd = IOS_IOCTLV;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	req->ioctlv.ioctl	= ioctl;
	req->ioctlv.argcin	= cnt_in;
	req->ioctlv.argcio	= cnt_io;
	req->ioctlv.argv	= (struct _ioctlv*)MEM_VIRTUAL_TO_PHYSICAL(argv);

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

/*
s32 iosCreateHeap(s32 size) {
	s32 i,ret;
	s32 free;
	u32 level;
	u32 ipclo,ipchi;
#ifdef DEBUG_IPC
	exiPrintf("iosCreateHeap(%d)\n",size);
#endif
	_CPU_ISR_Disable(level);

	i=0;
	while(i<IPC_NUMHEAPS) {
		if(_ipc_heaps[i].membase==NULL) break;
		i++;
	}
	if(i>=IPC_NUMHEAPS) {
		_CPU_ISR_Restore(level);
		return IPC_ENOHEAP;
	}

	ipclo = (((u32)IPC_GetBufferLo()+0x1f)&~0x1f);
	ipchi = (u32)IPC_GetBufferHi();
	free = (ipchi - (ipclo + size));
	if(free<0) return IPC_ENOMEM;

	_ipc_heaps[i].membase = (void*)ipclo;
	_ipc_heaps[i].size = size;

	ret = __lwp_heap_init(&_ipc_heaps[i].heap,(void*)ipclo,size,PPC_CACHE_ALIGNMENT);
	if(ret<=0) return IPC_ENOMEM;

	IPC_SetBufferLo((void*)(ipclo+size));
	_CPU_ISR_Restore(level);
	return i;
}

void* iosAlloc(s32 hid,s32 size) {
#ifdef DEBUG_IPC
	exiPrintf("iosAlloc(%d,%d)\n",hid,size);
#endif
	if(hid<0 || hid>=IPC_NUMHEAPS || size<=0) return NULL;
	return __lwp_heap_allocate(&_ipc_heaps[hid].heap,size);
}

void iosFree(s32 hid,void *ptr) {
#ifdef DEBUG_IPC
	exiPrintf("iosFree(%d,0x%p)\n",hid,ptr);
#endif
	if(hid<0 || hid>=IPC_NUMHEAPS || ptr==NULL) return;
	__lwp_heap_free(&_ipc_heaps[hid].heap,ptr);
}
*/
