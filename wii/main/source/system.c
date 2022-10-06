#include "main.h"

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
