/*-------------------------------------------------------------

ipc.c -- Interprocess Communication with Starlet

Copyright (C) 2008
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)
Hector Martin (marcan)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#if defined(HW_RVL)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <gcutil.h>
#include "main.h"
#include "asm.h"
#include "processor.h"
//#include "lwp.h"
#include "irq.h"
#include "ipc.h"
#include "cache.h"
#include "system.h"
#include "lwp_heap.h"
#include "lwp_wkspace.h"

//#define DEBUG_IPC
void exiPrintf(const char *fmt, ...);

#define IPC_HEAP_SIZE			4096
#define IPC_REQUESTSIZE			64
#define IPC_NUMHEAPS			16
#define IOS_MAXFMT_PARAMS		32
#define RELNCH_RELAUNCH 1
#define RELNCH_BACKGROUND 2

static OSMutex _ipcMutex;

struct _ipcreq
{						//ipc struct size: 32
	u32 cmd;			//0
	s32 result;			//4
	union {				//8
		s32 fd;
		u32 req_cmd;
	};
	union {
		struct {
			char *filepath;
			u32 mode;
		} open;
		struct {
			void *data;
			u32 len;
		} read, write;
		struct {
			s32 where;
			s32 whence;
		} seek;
		struct {
			u32 ioctl;
			void *buffer_in;
			u32 len_in;
			void *buffer_io;
			u32 len_io;
		} ioctl;
		struct {
			u32 ioctl;
			u32 argcin;
			u32 argcio;
			struct _ioctlv *argv;
		} ioctlv;
		u32 args[5];
	};

	ipccallback cb;		//32
	void *usrdata;		//36
	u32 relnch;			//40
	OSThreadQueue *syncqueue;	//44
	u32 magic;			//48 - used to avoid spurious responses, like from zelda.
	s32 serial;         //52 - for debug
	u8 pad1[8]; 		//56 - 60
};

//debug stuff
static const char *cmdNames[] = {
	"INVAL", "OPEN", "CLOSE", "READ", "WRITE", "SEEK", "IOCTL",
	"IOCTLV", "DONE"};
#define MAX_SERIAL_LOG 256
volatile u32 pendingSerials[MAX_SERIAL_LOG];
volatile struct _ipcreq *serialReqs[MAX_SERIAL_LOG];
volatile u32 nextSerial = 0;

void printReq(struct _ipcreq *req) {
	exiPrintf("IPC req @%08x: cmd=%d (%-6s) res=0x%08x fd=%d reqcmd=%d (%-6s)\r\n",
		req, req->cmd,
		req->cmd >= 0 && req->cmd < 9 ? cmdNames[req->cmd] : "???",
		req->result, req->fd, req->req_cmd,
		req->req_cmd >= 0 && req->req_cmd < 9 ? cmdNames[req->req_cmd] : "???");
	exiPrintf("cb=0x%08x udata=0x%08x relnch=%d queue=0x%08x ser=%d\r\n",
		req->cb, req->usrdata, req->relnch, req->syncqueue, req->serial);
	int cmd = req->cmd;
	if(cmd == 8) cmd = req->req_cmd;
	switch(cmd) {
		case 1: //open
			exiPrintf("path=%s mode=0x%X\r\n",
				PTR_VALID(req->open.filepath) ? req->open.filepath : "<INVALID>",
				req->open.mode);
			break;
		case 2: //close
			break;
		case 3: //read
			exiPrintf("into 0x%x, len %d\r\n", req->read.data, req->read.len);
			break;
		case 4: //write
			exiPrintf("from 0x%x, len %d\r\n", req->write.data, req->write.len);
			break;
		case 5: //seek
			exiPrintf("where=0x%x whence=0x%x\r\n", req->seek.where,
				req->seek.whence);
			break;
		case 6: //ioctl
			exiPrintf("ioctl=%d in=0x%x (%d) io=0x%x (%d)\r\n",
				req->ioctl.ioctl,
				req->ioctl.buffer_in, req->ioctl.len_in,
				req->ioctl.buffer_io, req->ioctl.len_io);
			break;
		case 7: { //ioctlv
			exiPrintf("ioctl=%d in=%d io=%d argv=0x%x (%d)\r\n",
				req->ioctlv.ioctl, req->ioctlv.argcin, req->ioctlv.argcio,
				PTR_VALID(req->ioctlv.argv) ? req->ioctlv.argv->data : 0,
				PTR_VALID(req->ioctlv.argv) ? req->ioctlv.argv->len  : 0);
			if(PTR_VALID(req->ioctlv.argv) && PTR_VALID(req->ioctlv.argv->data)) {
				exiPuts("data:");
				u8 *data = (u8*)req->ioctlv.argv->data;
				for(u32 i=0; i<req->ioctlv.argv->len; i++) {
					exiPrintf(" %02X", data[i]);
				}
				exiPuts("\r\n");
			}
			break;
		}
	}
	exiPuts("RAW DATA:\r\n");
	u8 *data = (u8*)req;
	for(int i=0; i<64; i++) {
		exiPrintf("%02X%s", data[i],
			((i & 0xF) == 0xF) ? "\r\n" : (
				((i & 3) == 3) ? " " : ""
			)
		);
	}
}
#if 0
void initSerials() {
	memset(pendingSerials, 0, sizeof(pendingSerials));
	memset(serialReqs, 0, sizeof(serialReqs));
}
u32 newSerial(struct _ipcreq *req) {
	if(req->serial) return req->serial;
	memset(req->pad1, 0, 8);
	u32 res = ++nextSerial;
	for(u32 i=0; i<MAX_SERIAL_LOG; i++) {
		if(pendingSerials[i] == 0) {
			pendingSerials[i] = res;
			serialReqs[i] = req;
			return res;
		}
	}
	exiPrintf("IPC SERIAL OVERFLOW: %d\r\n", res);
	return res;
}
void freeSerial(struct _ipcreq *req) {
	if(!(req && req->serial > 0)) return;
	for(u32 i=0; i<MAX_SERIAL_LOG; i++) {
		if(pendingSerials[i] == req->serial) {
			pendingSerials[i] = 0;
			serialReqs[i] = NULL;
			req->serial = 0;
			return;
		}
	}
	exiPrintf("IPC serial not found: %d\r\n", req->serial);
	printReq(req);
}
void checkSerials() {
	u32 count = 0;
	for(u32 i=0; i<MAX_SERIAL_LOG; i++) {
		if(pendingSerials[i]) count++;
	}
	if(count < 2) return;
	exiPuts("pending reqs:\r\n");
	for(u32 i=0; i<MAX_SERIAL_LOG; i++) {
		if(pendingSerials[i]) {
			struct _ipcreq *req = serialReqs[i];
			exiPrintf("%3d: 0x%08X: 0x%08X %-6s 0x%08X %d\r\n", pendingSerials[i],
				req, req->cmd,
				req->cmd >= 0 && req->cmd < 9 ? cmdNames[req->cmd] : "???",
				req->result, req->fd);
			//printReq(req);
		}
	}
	//exiPuts("\r\n");
}
#else
void initSerials() {}
u32 newSerial(struct _ipcreq *req) { return 0; }
void freeSerial(struct _ipcreq *req) {}
void checkSerials() {}
#endif
//end debug stuff

struct _ipcreqres
{
	u32 cnt_sent;
	u32 cnt_queue;
	u32 req_send_no;
	u32 req_queue_no;
	struct _ipcreq *reqs[16];
};

struct _ipcheap
{
	void *membase;
	u32 size;
	heap_cntrl heap;
};

struct _ioctlvfmt_bufent
{
	void *ipc_buf;
	void *io_buf;
	s32 copy_len;
};

struct _ioctlvfmt_cbdata
{
	ipccallback user_cb;
	void *user_data;
	s32 num_bufs;
	u32 hId;
	struct _ioctlvfmt_bufent *bufs;
};

static u32 IPC_REQ_MAGIC;

static s32 _ipc_hid = -1;
static s32 _ipc_mailboxack = 1;
static u32 _ipc_relnchFl = 0;
static u32 _ipc_initialized = 0;
static u32 _ipc_clntinitialized = 0;
static u64 _ipc_spuriousresponsecnt = 0;
static struct _ipcreq *_ipc_relnchRpc = NULL;

static void *_ipc_bufferlo = NULL;
static void *_ipc_bufferhi = NULL;
static void *_ipc_currbufferlo = NULL;
static void *_ipc_currbufferhi = NULL;

static u32 _ipc_seed = 0xffffffff;

static struct _ipcreqres _ipc_responses;

static struct _ipcheap _ipc_heaps[IPC_NUMHEAPS] =
{
	{NULL, 0, {}} // all other elements should be inited to zero, says C standard, so this should do
};

//static vu32* const _ipcReg = (u32*)0xCD000000;

extern void __MaskIrq(u32 nMask);
extern void __UnmaskIrq(u32 nMask);
extern void* __SYS_GetIPCBufferLo(void);
extern void* __SYS_GetIPCBufferHi(void);

extern u32 gettick(void);

static void _lockIpcMutex() {
	if(OSGetCurrentThread()) OSLockMutex(&_ipcMutex);
}
static void _unlockIpcMutex() {
	if(OSGetCurrentThread()) OSUnlockMutex(&_ipcMutex);
}

static __inline__ u32 IPC_ReadReg(u32 reg)
{
	return _ipcReg[reg];
}

static __inline__ void IPC_WriteReg(u32 reg,u32 val)
{
	_ipcReg[reg] = val;
}

static __inline__ void ACR_WriteReg(u32 reg,u32 val)
{
	_ipcReg[reg>>2] = val;
}

static __inline__ void* __ipc_allocreq(void)
{
	struct _ipcreq *req;
	req = iosAlloc(_ipc_hid,IPC_REQUESTSIZE);
	if(req) {
		req->serial = 0;
		newSerial(req);
	}
	return req;
}

static __inline__ void __ipc_freereq(void *ptr)
{
	struct _ipcreq *req = ptr;
	freeSerial(req);
	iosFree(_ipc_hid,ptr);
}

static __inline__ void __ipc_srand(u32 seed)
{
	_ipc_seed = seed;
}

static __inline__ u32 __ipc_rand(void)
{
	_ipc_seed = (214013*_ipc_seed) + 2531011;
	return _ipc_seed;
}

static s32 __ioctlvfmtCB(s32 result,void *userdata)
{
	ipccallback user_cb;
	void *user_data;
	struct _ioctlvfmt_cbdata *cbdata;
	struct _ioctlvfmt_bufent *pbuf;

	cbdata = (struct _ioctlvfmt_cbdata*)userdata;

	// deal with data buffers
	if(cbdata->bufs) {
		pbuf = cbdata->bufs;
		while(cbdata->num_bufs--) {
			if(pbuf->ipc_buf) {
				// copy data if needed
				if(pbuf->io_buf && pbuf->copy_len)
					memcpy(pbuf->io_buf, pbuf->ipc_buf, pbuf->copy_len);
				// then free the buffer
				iosFree(cbdata->hId, pbuf->ipc_buf);
			}
			pbuf++;
		}
	}

	user_cb = cbdata->user_cb;
	user_data = cbdata->user_data;

	// free buffer list
	__lwp_wkspace_free(cbdata->bufs);

	// free callback data
	__lwp_wkspace_free(cbdata);

	// call the user callback
	if(user_cb)
		return user_cb(result, user_data);

	return result;
}

static s32 __ipc_queuerequest(struct _ipcreq *req)
{
	u32 cnt;
	u32 level;
#ifdef DEBUG_IPC
	exiPrintf("__ipc_queuerequest(%p)\r\n",req);
#endif
	level = OSDisableInterrupts();
	newSerial(req);

	cnt = (_ipc_responses.cnt_queue - _ipc_responses.cnt_sent);
	if(cnt>=16) {
		OSRestoreInterrupts(level);
		return IPC_EQUEUEFULL;
	}

	_ipc_responses.reqs[_ipc_responses.req_queue_no] = req;
	_ipc_responses.req_queue_no = ((_ipc_responses.req_queue_no+1)&0x0f);
	_ipc_responses.cnt_queue++;

	OSRestoreInterrupts(level);
	return IPC_OK;
}

static s32 __ipc_syncqueuerequest(struct _ipcreq *req)
{
	u32 cnt;
#ifdef DEBUG_IPC
	exiPrintf("__ipc_syncqueuerequest(%p)\r\n",req);
#endif
	cnt = (_ipc_responses.cnt_queue - _ipc_responses.cnt_sent);
	if(cnt>=16) {
		return IPC_EQUEUEFULL;
	}

	newSerial(req);
	_ipc_responses.reqs[_ipc_responses.req_queue_no] = req;
	_ipc_responses.req_queue_no = ((_ipc_responses.req_queue_no+1)&0x0f);
	_ipc_responses.cnt_queue++;

	return IPC_OK;
}

static void __ipc_sendrequest(void)
{
	u32 cnt;
	u32 ipc_send;
	struct _ipcreq *req;
#ifdef DEBUG_IPC
	exiPrintf("__ipc_sendrequest()\r\n");
#endif
	cnt = (_ipc_responses.cnt_queue - _ipc_responses.cnt_sent);
	if(cnt>0) {
		req = _ipc_responses.reqs[_ipc_responses.req_send_no];
		//exiPrintf("IPC req %08X\n", req);
		if(req!=NULL) {
			newSerial(req);
			req->magic = IPC_REQ_MAGIC;
			if(req->relnch&RELNCH_RELAUNCH) {
				_ipc_relnchFl = 1;
				_ipc_relnchRpc = req;
				if(!(req->relnch&RELNCH_BACKGROUND))
					_ipc_mailboxack--;
			}
			DCFlushRange(req,sizeof(struct _ipcreq));

			IPC_WriteReg(0,MEM_VIRTUAL_TO_PHYSICAL(req));
			_ipc_responses.req_send_no = ((_ipc_responses.req_send_no+1)&0x0f);
			_ipc_responses.cnt_sent++;
			_ipc_mailboxack--;

			ipc_send = ((IPC_ReadReg(1)&0x30)|0x01);
			IPC_WriteReg(1,ipc_send);
		}
	}
}

static void __ipc_replyhandler(void)
{
	u32 ipc_ack,cnt;
	struct _ipcreq *req = NULL;
	ioctlv *v = NULL;
#ifdef DEBUG_IPC
	exiPuts("__ipc_replyhandler()\r\n");
#endif
	req = (struct _ipcreq*)IPC_ReadReg(2);
	if(req==NULL) return;

	ipc_ack = ((IPC_ReadReg(1)&0x30)|0x04); //clear IY1, IY2, Y1 (w1c)
	IPC_WriteReg(1,ipc_ack);
	ACR_WriteReg(48,0x40000000);

	req = MEM_PHYSICAL_TO_K0(req);
	DCInvalidateRange(req,32);

	freeSerial(req);

	if(req->magic==IPC_REQ_MAGIC) {
#ifdef DEBUG_IPC
		exiPrintf("IPC res: cmd %08x rcmd %08x res %08x\r\n",req->cmd,req->req_cmd,req->result);
#endif
		if(req->req_cmd==IOS_READ) {
			if(req->read.data!=NULL) {
				req->read.data = MEM_PHYSICAL_TO_K0(req->read.data);
				if(req->result>0) DCInvalidateRange(req->read.data,req->result);
			}
		} else if(req->req_cmd==IOS_IOCTL) {
			if(req->ioctl.buffer_io!=NULL) {
				req->ioctl.buffer_io = MEM_PHYSICAL_TO_K0(req->ioctl.buffer_io);
				DCInvalidateRange(req->ioctl.buffer_io,req->ioctl.len_io);
			}
			DCInvalidateRange(req->ioctl.buffer_in,req->ioctl.len_in);
		} else if(req->req_cmd==IOS_IOCTLV) {
			if(req->ioctlv.argv!=NULL) {
				req->ioctlv.argv = MEM_PHYSICAL_TO_K0(req->ioctlv.argv);
				DCInvalidateRange(req->ioctlv.argv,((req->ioctlv.argcin+req->ioctlv.argcio)*sizeof(struct _ioctlv)));
			}

			cnt = 0;
			v = (ioctlv*)req->ioctlv.argv;
			while(cnt<(req->ioctlv.argcin+req->ioctlv.argcio)) {
				if(v[cnt].data!=NULL) {
					v[cnt].data = MEM_PHYSICAL_TO_K0(v[cnt].data);
					DCInvalidateRange(v[cnt].data,v[cnt].len);
				}
				cnt++;
			}
			if(_ipc_relnchFl && _ipc_relnchRpc==req) {
				_ipc_relnchFl = 0;
				if(_ipc_mailboxack<1) _ipc_mailboxack++;
			}

		}

		if(req->cb!=NULL) {
			#ifdef DEBUG_IPC
				exiPrintf("%08X->cb = %08X(%08X,%08X)\r\n",
					req, req->cb, req->result,req->usrdata);
			#endif
			req->cb(req->result,req->usrdata);
			__ipc_freereq(req);
		} else {
			#ifdef DEBUG_IPC
				exiPrintf("IPC wakeup 0x%x\n", req->syncqueue);
			#endif
			if(!req->syncqueue) PANIC("__ipc_replyhandler: NULL queue");
			OSWakeupThread(req->syncqueue);
			//XXX do we not free req?
		}
	} else {
		// NOTE: we really want to find out if this ever happens
		// and take steps to prevent it beforehand (because it will
		// clobber memory, among other things). I suggest leaving this in
		// even in non-DEBUG mode. Maybe even cause a system halt.
		// It is the responsibility of the loader to clear these things,
		// but we want to find out if they happen so loaders can be fixed.
//#ifdef DEBUG_IPC
		exiPrintf("Received unknown IPC response (magic %08x):\r\n", req->magic);
		exiPrintf("  CMD %08x RES %08x REQCMD %08x\r\n", req->cmd, req->result, req->req_cmd);
		exiPrintf("  Args: %08x %08x %08x %08x %08x\r\n", req->args[0], req->args[1], req->args[2], req->args[3], req->args[4]);
		exiPrintf("  CB %08x DATA %08x REL %08x QUEUE %08x\r\n", (u32)req->cb, (u32)req->usrdata, req->relnch, (u32)req->syncqueue);
//#endif
		_ipc_spuriousresponsecnt++;
	}
	ipc_ack = ((IPC_ReadReg(1)&0x30)|0x08);
	IPC_WriteReg(1,ipc_ack);
}

static void __ipc_ackhandler(void)
{
	u32 ipc_ack;
#ifdef DEBUG_IPC
	exiPrintf("__ipc_ackhandler()\r\n");
#endif
	ipc_ack = ((IPC_ReadReg(1)&0x30)|0x02); //clear IY1, IY2, Y2 (w1c)
	IPC_WriteReg(1,ipc_ack);
	ACR_WriteReg(48,0x40000000); //mystery register

	if(_ipc_mailboxack<1) _ipc_mailboxack++;
	if(_ipc_mailboxack>0) {
		if(_ipc_relnchFl){
			_ipc_relnchRpc->result = 0;
			_ipc_relnchFl = 0;

			OSWakeupThread(_ipc_relnchRpc->syncqueue);

			ipc_ack = ((IPC_ReadReg(1)&0x30)|0x08);
			IPC_WriteReg(1,ipc_ack);
		}
		__ipc_sendrequest();
	}

}

void __ipc_interrupthandler(u32 irq,void *ctx)
{
	u32 ipc_int;
//#ifdef DEBUG_IPC
//	exiPrintf("__ipc_interrupthandler(%d)\r\n",irq);
//#endif
	//SET_DEBUG_PORT(1);
	//iguanaSetBlueLed(true);
	ipc_int = IPC_ReadReg(1);
	//IY1 | Y1 (command was executed)
	if((ipc_int&0x0014)==0x0014) __ipc_replyhandler();
	//iguanaSetBlueLed(false);
	//iguanaSetGreenLed(true);

	ipc_int = IPC_ReadReg(1);
	//IY2 | Y2 (command acknowledged)
	if((ipc_int&0x0022)==0x0022) __ipc_ackhandler();
	//SET_DEBUG_PORT(0);
	//iguanaSetGreenLed(false);

	checkSerials();
}

static s32 __ios_ioctlvformat_parse(const char *format,va_list args,struct _ioctlvfmt_cbdata *cbdata,s32 *cnt_in,s32 *cnt_io,struct _ioctlv **argv,s32 hId)
{
	s32 ret,i;
	void *pdata;
	void *iodata;
	char type,*ps;
	s32 len,maxbufs = 0;
	ioctlv *argp = NULL;
	struct _ioctlvfmt_bufent *bufp;

	if(hId == IPC_HEAP) hId = _ipc_hid;
	if(hId < 0) return IPC_EINVAL;

	maxbufs = strnlen(format,IOS_MAXFMT_PARAMS);
	if(maxbufs>=IOS_MAXFMT_PARAMS) return IPC_EINVAL;

	cbdata->hId = hId;
	cbdata->bufs = __lwp_wkspace_allocate((sizeof(struct _ioctlvfmt_bufent)*(maxbufs+1)));
	if(cbdata->bufs==NULL) return IPC_ENOMEM;

	argp = iosAlloc(hId,(sizeof(struct _ioctlv)*(maxbufs+1)));
	if(argp==NULL) {
		__lwp_wkspace_free(cbdata->bufs);
		return IPC_ENOMEM;
	}

	*argv = argp;
	bufp = cbdata->bufs;
	memset(argp,0,(sizeof(struct _ioctlv)*(maxbufs+1)));
	memset(bufp,0,(sizeof(struct _ioctlvfmt_bufent)*(maxbufs+1)));

	cbdata->num_bufs = 1;
	bufp->ipc_buf = argp;
	bufp++;

	*cnt_in = 0;
	*cnt_io = 0;

	ret = IPC_OK;
	while(*format) {
		type = tolower((int)*format);
		switch(type) {
			case 'b':
				pdata = iosAlloc(hId,sizeof(u8));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u8*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u8);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'h':
				pdata = iosAlloc(hId,sizeof(u16));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u16*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u16);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'i':
				pdata = iosAlloc(hId,sizeof(u32));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u32*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u32);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'q':
				pdata = iosAlloc(hId,sizeof(u64));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u64*)pdata = va_arg(args,u64);
				argp->data = pdata;
				argp->len = sizeof(u64);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'd':
				argp->data = va_arg(args, void*);
				argp->len = va_arg(args, u32);
				(*cnt_in)++;
				argp++;
				break;
			case 's':
				ps = va_arg(args, char*);
				len = strnlen(ps,256);
				if(len>=256) {
					ret = IPC_EINVAL;
					goto free_and_error;
				}

				pdata = iosAlloc(hId,(len+1));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				memcpy(pdata,ps,(len+1));
				argp->data = pdata;
				argp->len = (len+1);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case ':':
				format++;
				goto parse_io_params;
			default:
				ret = IPC_EINVAL;
				goto free_and_error;
		}
		format++;
	}

parse_io_params:
	while(*format) {
		type = tolower((int)*format);
		switch(type) {
			case 'b':
				pdata = iosAlloc(hId,sizeof(u8));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u8*);
				*(u8*)pdata = *(u8*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u8);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u8);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'h':
				pdata = iosAlloc(hId,sizeof(u16));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u16*);
				*(u16*)pdata = *(u16*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u16);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u16);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'i':
				pdata = iosAlloc(hId,sizeof(u32));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u32*);
				*(u32*)pdata = *(u32*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u32);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u32);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'q':
				pdata = iosAlloc(hId,sizeof(u64));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u64*);
				*(u64*)pdata = *(u64*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u64);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u64);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'd':
				argp->data = va_arg(args, void*);
				argp->len = va_arg(args, u32);
				(*cnt_io)++;
				argp++;
				break;
			default:
				ret = IPC_EINVAL;
				goto free_and_error;
		}
		format++;
	}
	return IPC_OK;

free_and_error:
	for(i=0;i<cbdata->num_bufs;i++) {
		if(cbdata->bufs[i].ipc_buf!=NULL) iosFree(hId,cbdata->bufs[i].ipc_buf);
	}
	__lwp_wkspace_free(cbdata->bufs);
	return ret;
}

static s32 __ipc_asyncrequest(struct _ipcreq *req)
{
	s32 ret;
	u32 level;

	ret = __ipc_queuerequest(req);
	if(ret) __ipc_freereq(req);
	else {
		level = OSDisableInterrupts();
		if(_ipc_mailboxack>0) __ipc_sendrequest();
		OSRestoreInterrupts(level);
	}
	return ret;
}

static s32 __ipc_syncrequest(struct _ipcreq *req)
{
	s32 ret;
	u32 level;

	//exiPrintf("%s: init queue\n", __FUNCTION__);
	level = OSDisableInterrupts();
	//req->syncqueue = malloc(sizeof(OSThreadQueue));
	req->syncqueue = iosAlloc(_ipc_hid, sizeof(OSThreadQueue));
	if(!req->syncqueue) {
		OSRestoreInterrupts(level);
		return -ENOMEM;
	}
	OSInitThreadQueue(req->syncqueue);

	//exiPrintf("%s: disable IRQ\n", __FUNCTION__);
	ret = __ipc_syncqueuerequest(req);
	//exiPrintf("IPC sync req done\n");
	//udelay(10000);
	if(ret==0) {
		if(_ipc_mailboxack>0) {
			//exiPrintf("IPC sync ACK\n");
			__ipc_sendrequest();
			//udelay(5000);
		}
		#ifdef DEBUG_IPC
			exiPrintf("IPC sync sleeping(%08X)\n", req->syncqueue);
		#endif
		//SET_DEBUG_PORT(1);
		OSSleepThread(req->syncqueue);
		//SET_DEBUG_PORT(0);
		#ifdef DEBUG_IPC
			exiPrintf("IPC sync sleep done(%08X)\n", req->syncqueue);
		#endif
		ret = req->result;
	}
	else {
		exiPrintf("IPC sync ERROR %d\n", ret);
	}

	//free(req->syncqueue);
	freeSerial(req);
	iosFree(_ipc_hid, req->syncqueue);
	req->syncqueue = NULL;
	OSRestoreInterrupts(level);
	return ret;
}

s32 iosCreateHeap(s32 size)
{
	s32 i,ret;
	s32 free;
	u32 level;
	u32 ipclo,ipchi;
#ifdef DEBUG_IPC
	exiPrintf("iosCreateHeap(%d) from %08x\r\n",size,
		__builtin_return_address(0));
#endif
	level = OSDisableInterrupts();

	i=0;
	while(i<IPC_NUMHEAPS) {
		if(_ipc_heaps[i].membase==NULL) break;
		i++;
	}
	if(i>=IPC_NUMHEAPS) {
		OSRestoreInterrupts(level);
		return IPC_ENOHEAP;
	}

	ipclo = (((u32)IPC_GetBufferLo()+0x1f)&~0x1f);
	ipchi = (u32)IPC_GetBufferHi();
	free = (ipchi - (ipclo + size));
	if(free<0) {
		exiPrintf(" *** iosCreateHeap(%d) failed (line %d)\n", size, __LINE__);
		return IPC_ENOMEM;
	}

	_ipc_heaps[i].membase = (void*)ipclo;
	_ipc_heaps[i].size = size;

	ret = __lwp_heap_init(&_ipc_heaps[i].heap,(void*)ipclo,size,PPC_CACHE_ALIGNMENT);
	if(ret<=0) {
		exiPrintf(" *** iosCreateHeap(%d) failed (line %d)\n", size, __LINE__);
		return IPC_ENOMEM;
	}

	IPC_SetBufferLo((void*)(ipclo+size));
	OSRestoreInterrupts(level);
	return i;
}

void* iosAlloc(s32 hid,s32 size)
{
#ifdef DEBUG_IPC
	exiPrintf("iosAlloc(%d,%d)\r\n",hid,size);
#endif
	if(hid<0 || hid>=IPC_NUMHEAPS || size<=0) return NULL;
	int level = OSDisableInterrupts();
	void *r = __lwp_heap_allocate(&_ipc_heaps[hid].heap,size);
	OSRestoreInterrupts(level);
	return r;
}

void iosFree(s32 hid,void *ptr)
{
#ifdef DEBUG_IPC
	exiPrintf("iosFree(%d,%p)\r\n",hid,ptr);
#endif
	if(hid<0 || hid>=IPC_NUMHEAPS || ptr==NULL) return;
	int level = OSDisableInterrupts();
	__lwp_heap_free(&_ipc_heaps[hid].heap,ptr);
	OSRestoreInterrupts(level);
#ifdef DEBUG_IPC
	exiPrintf("iosFree(%d,%p) OK\r\n",hid,ptr);
#endif
}

void* IPC_GetBufferLo(void)
{
	return _ipc_currbufferlo;
}

void* IPC_GetBufferHi(void)
{
	return _ipc_currbufferhi;
}

void IPC_SetBufferLo(void *bufferlo)
{
	if(_ipc_bufferlo<=bufferlo) _ipc_currbufferlo = bufferlo;
}

void IPC_SetBufferHi(void *bufferhi)
{
	if(bufferhi<=_ipc_bufferhi) _ipc_currbufferhi = bufferhi;
}

void __IPC_Init(void)
{
	if(!_ipc_initialized) {
		_ipc_bufferlo = _ipc_currbufferlo = __SYS_GetIPCBufferLo();
		_ipc_bufferhi = _ipc_currbufferhi = __SYS_GetIPCBufferHi();
		_ipc_initialized = 1;
		OSInitMutex(&_ipcMutex);
	}
}

u32 __IPC_ClntInit(void)
{
	if(!_ipc_clntinitialized) {
		_ipc_clntinitialized = 1;
		initSerials();

		// generate a random request magic
		__ipc_srand(gettick());
		IPC_REQ_MAGIC = __ipc_rand();
		__IPC_Init();

		_ipc_hid = iosCreateHeap(IPC_HEAP_SIZE);
		//IRQ_Request(IRQ_PI_ACR,__ipc_interrupthandler,NULL);
		// *(void**)0x800030ac = __ipc_interrupthandler;
		gameIrqHandlers[27] = __ipc_interrupthandler;
		__UnmaskIrq(IM_PI_ACR);
		IPC_WriteReg(1,56);
	}
	return IPC_OK;
}

void __IPC_Reinitialize(void)
{
	u32 level;

	exiPuts("__IPC_Reinitialize\r\n");
	level = OSDisableInterrupts();

	IPC_WriteReg(1,56);

	_ipc_mailboxack = 1;
	_ipc_relnchFl = 0;
	_ipc_relnchRpc = NULL;

	_ipc_responses.req_queue_no = 0;
	_ipc_responses.cnt_queue = 0;
	_ipc_responses.req_send_no = 0;
	_ipc_responses.cnt_sent = 0;

	OSRestoreInterrupts(level);
}

s32 IOS_Open(const char *filepath,u32 mode)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	//exiPrintf("%s(%s, %08X)\n", __FUNCTION__, filepath, mode);
	if(filepath==NULL) {
		_unlockIpcMutex();
		return IPC_EINVAL;
	}
	/*if(!PTR_VALID(filepath)) {
		exiPrintf(" *** ERROR *** Invalid IOS_Open(%p)\n", filepath);
		*(u32*)0 = 0;
	}*/

	req = __ipc_allocreq();
	if(req==NULL) {
		exiPrintf(" *** ERROR *** %s: alloc failed\n", __FUNCTION__);
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_OPEN;
	req->cb = NULL;
	req->relnch = 0;

	//exiPrintf("%s(%s, %08X) path=%p req=%p\n", __FUNCTION__,
	//	filepath, mode, filepath, req);
	DCFlushRange((void*)filepath,strnlen(filepath,IPC_MAXPATH_LEN) + 1);

	req->open.filepath	= (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req->open.mode		= mode;

	//exiPrintf("%s: __ipc_syncrequest...\n", __FUNCTION__);
	ret = __ipc_syncrequest(req);
	//exiPrintf("%s: __ipc_syncrequest done\n", __FUNCTION__);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_OpenAsync(const char *filepath,u32 mode,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_OPEN;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCFlushRange((void*)filepath,strnlen(filepath,IPC_MAXPATH_LEN) + 1);

	req->open.filepath	= (char*)MEM_VIRTUAL_TO_PHYSICAL(filepath);
	req->open.mode		= mode;

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Close(s32 fd)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_CLOSE;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = 0;

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_CloseAsync(s32 fd,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_CLOSE;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Read(s32 fd,void *buf,s32 len)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_READ;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = 0;

	DCInvalidateRange(buf,len);
	req->read.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->read.len	= len;

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_ReadAsync(s32 fd,void *buf,s32 len,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_READ;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCInvalidateRange(buf,len);
	req->read.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->read.len	= len;

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Write(s32 fd,const void *buf,s32 len)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_WRITE;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = 0;

	DCFlushRange((void*)buf,len);
	req->write.data	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->write.len	= len;

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_WriteAsync(s32 fd,const void *buf,s32 len,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_WRITE;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	DCFlushRange((void*)buf,len);
	req->write.data		= (void*)MEM_VIRTUAL_TO_PHYSICAL(buf);
	req->write.len		= len;

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Seek(s32 fd,s32 where,s32 whence)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_SEEK;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = 0;

	req->seek.where		= where;
	req->seek.whence	= whence;

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_SeekAsync(s32 fd,s32 where,s32 whence,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_SEEK;
	req->fd = fd;
	req->cb = ipc_cb;
	req->usrdata = usrdata;
	req->relnch = 0;

	req->seek.where		= where;
	req->seek.whence	= whence;

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Ioctl(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io)
{
	s32 ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_IOCTL;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = 0;

	req->ioctl.ioctl		= ioctl;
	req->ioctl.buffer_in	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_in);
	req->ioctl.len_in		= len_in;
	req->ioctl.buffer_io	= (void*)MEM_VIRTUAL_TO_PHYSICAL(buffer_io);
	req->ioctl.len_io		= len_io;

	if(buffer_in) DCFlushRange(buffer_in,len_in);
	if(buffer_io) DCFlushRange(buffer_io,len_io);

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_IoctlAsync(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io,ipccallback ipc_cb,void *usrdata)
{
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	memset(req->pad1, 0, 8);
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

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_Ioctlv(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv)
{
	s32 i,ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_IOCTLV;
	req->fd = fd;
	req->cb = NULL;
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

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}


s32 IOS_IoctlvAsync(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv,ipccallback ipc_cb,void *usrdata)
{
	s32 i;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

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

	s32 r = __ipc_asyncrequest(req);
	_unlockIpcMutex();
	return r;
}

s32 IOS_IoctlvFormat(s32 hId,s32 fd,s32 ioctl,const char *format,...)
{
	s32 ret;
	va_list args;
	s32 cnt_in,cnt_io;
	struct _ioctlv *argv;
	struct _ioctlvfmt_cbdata *cbdata;

	_lockIpcMutex();
	cbdata = __lwp_wkspace_allocate(sizeof(struct _ioctlvfmt_cbdata));
	if(cbdata==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	memset(cbdata,0,sizeof(struct _ioctlvfmt_cbdata));

	va_start(args,format);
	ret = __ios_ioctlvformat_parse(format,args,cbdata,&cnt_in,&cnt_io,&argv,hId);
	va_end(args);
	if(ret<0) {
		__lwp_wkspace_free(cbdata);
		_unlockIpcMutex();
		return ret;
	}

	ret = IOS_Ioctlv(fd,ioctl,cnt_in,cnt_io,argv);
	__ioctlvfmtCB(ret,cbdata);

	_unlockIpcMutex();
	return ret;
}

s32 IOS_IoctlvFormatAsync(s32 hId,s32 fd,s32 ioctl,ipccallback usr_cb,void *usr_data,const char *format,...)
{
	s32 ret;
	va_list args;
	s32 cnt_in,cnt_io;
	struct _ioctlv *argv;
	struct _ioctlvfmt_cbdata *cbdata;

	_lockIpcMutex();
	cbdata = __lwp_wkspace_allocate(sizeof(struct _ioctlvfmt_cbdata));
	if(cbdata==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	memset(cbdata,0,sizeof(struct _ioctlvfmt_cbdata));

	va_start(args,format);
	ret = __ios_ioctlvformat_parse(format,args,cbdata,&cnt_in,&cnt_io,&argv,hId);
	va_end(args);
	if(ret<0) {
		__lwp_wkspace_free(cbdata);
		_unlockIpcMutex();
		return ret;
	}

	cbdata->user_cb = usr_cb;
	cbdata->user_data = usr_data;
	s32 r = IOS_IoctlvAsync(fd,ioctl,cnt_in,cnt_io,argv,__ioctlvfmtCB,cbdata);
	_unlockIpcMutex();
	return r;
}

s32 IOS_IoctlvReboot(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv)
{
	s32 i,ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_IOCTLV;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = RELNCH_RELAUNCH;

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

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

s32 IOS_IoctlvRebootBackground(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv)
{
	s32 i,ret;
	struct _ipcreq *req;

	_lockIpcMutex();
	req = __ipc_allocreq();
	if(req==NULL) {
		_unlockIpcMutex();
		return IPC_ENOMEM;
	}

	req->cmd = IOS_IOCTLV;
	req->result = 0;
	req->fd = fd;
	req->cb = NULL;
	req->relnch = RELNCH_BACKGROUND|RELNCH_RELAUNCH;

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

	ret = __ipc_syncrequest(req);

	if(req!=NULL) __ipc_freereq(req);
	_unlockIpcMutex();
	return ret;
}

#endif
