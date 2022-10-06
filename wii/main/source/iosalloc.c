#include "main.h"

static s32 _ipc_hid = -1;
static s32 __ios_hid = -1;

void *_ipc_bufferlo = NULL;
void *_ipc_bufferhi = NULL;
void *_ipc_currbufferlo = NULL;
void *_ipc_currbufferhi = NULL;

IpcHeap _ipc_heaps[IPC_NUMHEAPS] = {
	{NULL, 0, {}} // all other elements should be inited to zero,
    //says C standard, so this should do
};

s32 iosCreateHeap(s32 size) {
	s32 i,ret;
	s32 free;
	u32 level;
	u32 ipclo,ipchi;
#if IPC_DEBUG
	exiPrintf("iosCreateHeap(%d)\n",size);
#endif
	_CPU_ISR_Disable(level);

	i=0;
	while(i<IPC_NUMHEAPS) {
		if(_ipc_heaps[i].membase==NULL) break;
		i++;
	}
	if(i >= IPC_NUMHEAPS) {
		_CPU_ISR_Restore(level);
        exiPrintf(" *** ERROR *** IPC: out of heaps!\n");
		return IPC_ENOHEAP;
	}

	ipclo = (((u32)IPC_GetBufferLo()+0x1f)&~0x1f);
	ipchi = (u32)IPC_GetBufferHi();
	free  = (ipchi - (ipclo + size));
	if(free<0) {
		exiPrintf(" *** ERROR *** IPC: out of space for new heap!\n");
		return IPC_ENOMEM;
	}

	_ipc_heaps[i].membase = (void*)ipclo;
	_ipc_heaps[i].size = size;

	ret = __lwp_heap_init(&_ipc_heaps[i].heap, (void*)ipclo, size,
        PPC_CACHE_ALIGNMENT);
	if(ret<=0) {
		exiPrintf(" *** ERROR *** IPC: heap init failed!\n");
		return IPC_ENOMEM;
	}

	IPC_SetBufferLo((void*)(ipclo+size));
	_CPU_ISR_Restore(level);
	return i;
}

void* iosAlloc(s32 hid,s32 size) {
    if(hid<0 || hid>=IPC_NUMHEAPS || size<=0) {
        exiPrintf(" *** ERROR *** using invalid IOS heap at %08X\n",
            RETURN_ADDRESS);
        return NULL;
    }
    IPC_DPRINT("IPC: iosAlloc(%d,%d) ", hid, size);
	void *r = __lwp_heap_allocate(&_ipc_heaps[hid].heap,size);
	//void *r = malloc(size);
	if(!r) {
		exiPrintf(" *** ERROR *** iosAlloc(%d,%d) FAILED at %08X\n",
			hid, size, RETURN_ADDRESS);
	}
    IPC_DPRINT("=> %p\n",r);
    return r;
}

void iosFree(s32 hid,void *ptr) {
    ptr = MEM_PHYSICAL_TO_VIRTUAL(ptr);
    IPC_DPRINT("iosFree(%d,%p) @%08x\n",hid,ptr,RETURN_ADDRESS);
	if(hid<0 || hid>=IPC_NUMHEAPS || ptr==NULL) return;
	__lwp_heap_free(&_ipc_heaps[hid].heap,ptr);
	//free(ptr);
}

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

IpcRequestAndMsgQueue* __ipc_allocreq(void) {
	//IpcRequestAndMsgQueue *req = iosAlloc(_ipc_hid,
	//	sizeof(IpcRequestAndMsgQueue));
	//if(!req) return NULL;
	int irq = OSDisableInterrupts();

	int next = (_ipcReqQueueHead + 1) % IPC_QUEUE_MAX;
	if(next == _ipcReqQueueTail) {
		OSRestoreInterrupts(irq);
        exiPrintf(" *** ERROR *** IPC outgoing request buffer full\n");
		return NULL;
	}

	IpcRequestAndMsgQueue *req = &_ipcRequestQueue[_ipcReqQueueHead];
    IPC_DPRINT("IPC: alloc new req at %08X (slot %d)\n",
        (u32)req, _ipcReqQueueHead);

    /*if(next <= _prevHead && next > 1) {
        exiPrintf(" *** ERROR *** IPC head corrupted (%d -> %d)\n",
            _prevHead, next);
    }*/

	req->ready = false;
	_ipcReqQueueHead = next;
	OSInitThreadQueue(&req->queue);
	OSRestoreInterrupts(irq);
	return req;
}

void __ipc_freereq(IpcRequestAndMsgQueue *ptr) {
    ptr->ready = false;
}

s32 __IOS_InitHeap(void) {
    if(_ipc_hid < 0) {
        _ipc_hid = iosCreateHeap(IPC_HEAP_SIZE);
    }
	if(__ios_hid <0 ) {
		__ios_hid = iosCreateHeap(IOS_HEAP_SIZE);
		if(__ios_hid < 0) return __ios_hid;
	}
	return 0;
}
