#include "main.h"

//static void *__sysarena2lo = NULL;
//static void *__sysarena2hi = NULL;
static void *__ipcbufferlo = NULL;
static void *__ipcbufferhi = NULL;
extern u8 __ipcbufferLo[], __ipcbufferHi[];

int main(int argc, char **argv);

void SYS_Init(void) {
    //called from crt0.S
    u32 level;
	_CPU_ISR_Disable(level);
    __ipcbufferlo = (void*)__ipcbufferLo; //what the fuck
	__ipcbufferhi = (void*)__ipcbufferHi;
    main(_argc, _argc_raw);
}

void* __SYS_GetIPCBufferLo(void) {
	return __ipcbufferlo;
}

void* __SYS_GetIPCBufferHi(void) {
	return __ipcbufferhi;
}

#if 0
void SYS_SetArena2Lo(void *newLo)
{
	u32 level;

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
#endif
