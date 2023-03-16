#include "main.h"

static void *__sysarena1lo = NULL;
static void *__sysarena1hi = NULL;
static void *__sysarena2lo = NULL;
static void *__sysarena2hi = NULL;
static void *__ipcbufferlo = NULL;
static void *__ipcbufferhi = NULL;
extern u8 __ipcbufferLo[], __ipcbufferHi[];

int main(int argc, char **argv);

#define SYS_PROTECTCHAN0				0			/*!< OS memory protection channel 0 */
#define SYS_PROTECTCHAN1				1			/*!< OS memory protection channel 1 */
#define SYS_PROTECTCHAN2				2			/*!< OS memory protection channel 2 */
#define SYS_PROTECTCHAN3				3			/*!< OS memory protection channel 2 */
#define SYS_PROTECTCHANMAX				4			/*!< _Termination */
#define KERNEL_HEAP					(1*1024*1024)

void SYS_Init(void) {
    //called from crt0.S
	SET_DEBUG_PORT(__LINE__);
	u32 level;
	_CPU_ISR_Disable(level);
	//__SYS_PreInit();
	//SET_DEBUG_PORT(__LINE__);
	//_V_EXPORTNAME();
	//__lowmem_init();
	SYS_SetArena1Lo((void*)0x92000000);
	SYS_SetArena1Hi((void*)0x92100000);
	SYS_SetArena2Lo((void*)0x92100000);
	SYS_SetArena2Hi((void*)0x93000000);
    __ipcbufferlo = (void*)__ipcbufferLo; //what the fuck
	__ipcbufferhi = (void*)__ipcbufferHi;
	//why would you have these differ only by one letter case
	SET_DEBUG_PORT(__LINE__);
	__lwp_wkspace_init(KERNEL_HEAP);
	//__lwp_priority_init();
	//__lwp_watchdog_init();
	SET_DEBUG_PORT(__LINE__);
	//__IPC_ClntInit();
	//SET_DEBUG_PORT(__LINE__);
	//__libc_init(1);
	//SET_DEBUG_PORT(__LINE__);
	//SYS_PreMain();
	SET_DEBUG_PORT(__LINE__);
    main(_argc, _argc_raw);
	SET_DEBUG_PORT(__LINE__);
}

void* __SYS_GetIPCBufferLo(void) {
	return __ipcbufferlo;
}

void* __SYS_GetIPCBufferHi(void) {
	return __ipcbufferhi;
}

void SYS_ProtectRange(u32 chan,void *addr,u32 bytes,u32 cntrl)
{
	u16 rcntrl;
	u32 pstart,pend,level;

	if(chan<SYS_PROTECTCHANMAX) {
		pstart = ((u32)addr)&~0x3ff;
		pend = ((((u32)addr)+bytes)+1023)&~0x3ff;
		DCFlushRange((void*)pstart,(pend-pstart));

		_CPU_ISR_Disable(level);

		__UnmaskIrq(IRQMASK(chan));
		_memReg[chan<<2] = _SHIFTR(pstart,10,16);
		_memReg[(chan<<2)+1] = _SHIFTR(pend,10,16);

		rcntrl = _memReg[8];
		rcntrl = (rcntrl&~(_SHIFTL(3,(chan<<1),2)))|(_SHIFTL(cntrl,(chan<<1),2));
		_memReg[8] = rcntrl;

		if(cntrl==SYS_PROTECTRDWR)
			__MaskIrq(IRQMASK(chan));


		_CPU_ISR_Restore(level);
	}
}

void SYS_SetArena1Lo(void *newLo)
{
	u32 level;

	_CPU_ISR_Disable(level);
	__sysarena1lo = newLo;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena1Lo(void)
{
	u32 level;
	void *arenalo;

	_CPU_ISR_Disable(level);
	arenalo = __sysarena1lo;
	_CPU_ISR_Restore(level);

	return arenalo;
}

void SYS_SetArena1Hi(void *newHi)
{
	u32 level;

	_CPU_ISR_Disable(level);
	__sysarena1hi = newHi;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena1Hi(void)
{
	u32 level;
	void *arenahi;

	_CPU_ISR_Disable(level);
	arenahi = __sysarena1hi;
	_CPU_ISR_Restore(level);

	return arenahi;
}

u32 SYS_GetArena1Size(void)
{
	u32 level,size;

	_CPU_ISR_Disable(level);
	size = ((u32)__sysarena1hi - (u32)__sysarena1lo);
	_CPU_ISR_Restore(level);

	return size;
}

void* SYS_AllocArena1MemLo(u32 size,u32 align)
{
	u32 mem1lo;
	void *ptr = NULL;

	mem1lo = (u32)SYS_GetArena1Lo();
	ptr = (void*)((mem1lo+(align-1))&~(align-1));
	mem1lo = ((((u32)ptr+size+align)-1)&~(align-1));
	SYS_SetArena1Lo((void*)mem1lo);

	return ptr;
}

void SYS_SetArena2Lo(void *newLo)
{
	u32 level;

	//exiPrintf("SetArena2Lo(%p)\n", newLo);
	//if(!PTR_VALID(newLo)) *(u32*)0 = 0xBAD;
	_CPU_ISR_Disable(level);
	__sysarena2lo = newLo;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena2Lo(void)
{
	u32 level;
	void *arenalo;

	_CPU_ISR_Disable(level);
	arenalo = __sysarena2lo;
	_CPU_ISR_Restore(level);

	return arenalo;
}

void SYS_SetArena2Hi(void *newHi)
{
	u32 level;

	//exiPrintf("SetArena2Hi(%p)\n", newHi);
	if(!PTR_VALID(newHi)) *(u32*)0 = 0xBAD;
	_CPU_ISR_Disable(level);
	__sysarena2hi = newHi;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena2Hi(void)
{
	u32 level;
	void *arenahi;

	_CPU_ISR_Disable(level);
	arenahi = __sysarena2hi;
	_CPU_ISR_Restore(level);

	return arenahi;
}

u32 SYS_GetArena2Size(void)
{
	u32 level,size;

	_CPU_ISR_Disable(level);
	size = ((u32)__sysarena2hi - (u32)__sysarena2lo);
	_CPU_ISR_Restore(level);

	return size;
}

void* SYS_AllocArena2MemLo(u32 size,u32 align)
{
	u32 mem2lo;
	void *ptr = NULL;

	mem2lo = (u32)SYS_GetArena2Lo();
	ptr = (void*)((mem2lo+(align-1))&~(align-1));
	mem2lo = ((((u32)ptr+size+align)-1)&~(align-1));
	SYS_SetArena2Lo((void*)mem2lo);

	return ptr;
}
