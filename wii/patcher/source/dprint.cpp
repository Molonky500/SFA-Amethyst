//adapted from Snes9xgx
#include "main.h"

static bool gecko = false;
static mutex_t gecko_mutex = 0;
static const int chan = 0;

static ssize_t __gecko_write(struct _reent *r, void* fd,
const char *ptr, size_t len) {
	if (!gecko || len == 0)
		return len;

	if(!ptr || len < 0)
		return -1;

	u32 level;
	LWP_MutexLock(gecko_mutex);
	level = IRQ_Disable();
	usb_sendbuffer(1, ptr, len);
	IRQ_Restore(level);
	LWP_MutexUnlock(gecko_mutex);
	return len;
}

static ssize_t __uart_write(struct _reent *r, void* fd,
const char *ptr, size_t len) {
	DCFlushRange((void*)ptr,len);

	if(EXI_Lock(EXI_CHANNEL_0,EXI_DEVICE_1,NULL)==0) return 0;
	if(EXI_Select(EXI_CHANNEL_0,EXI_DEVICE_1,EXI_SPEED8MHZ)==0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	static u32 addr = (0x800400 << 6) | 0x80000000;
	EXI_Imm     (EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync    (EXI_CHANNEL_0);
	//DMA requires 32-byte alignment
	//EXI_Dma     (EXI_CHANNEL_0, (void*)ptr, len, EXI_WRITE, NULL);
	char prev = 0;
	for(size_t i=0; i<len; i++) {
		char c = ptr[i];
		//this uart is weird, uses \r as "actually write"
		if(c == '\n') {
			if(prev == '\r') continue;
			c = '\r';
		}
		u32 data = (u32)c << 24;
		EXI_Imm(EXI_CHANNEL_0, &data, 1, EXI_WRITE, NULL);
		EXI_Sync(EXI_CHANNEL_0);
		prev = c;
	}
	EXI_Sync    (EXI_CHANNEL_0);
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock  (EXI_CHANNEL_0);

	return len;
}

const devoptab_t gecko_out = {
	"gecko",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__gecko_write,// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};

//the "real" UART which is present on devkits.
//Dolphin supports this, so use it when present.
const devoptab_t uart_out = {
	"uart",		// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__uart_write,// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};

void initDebugPrint() {
	//on emulator, always use uart
	u32 sysType = *(u32*)0x8000002c;
	if((sysType & 0xF0000000) //emulator/dev HW
	|| (sysType == 0x23)) { //???
		gecko = false;
	}
	else gecko = usb_isgeckoalive(1);
	//gecko = false; //lol
	LWP_MutexInit(&gecko_mutex, false);

	if(gecko) {
		devoptab_list[STD_OUT] = &gecko_out;
		devoptab_list[STD_ERR] = &gecko_out;
	}
	else {
		devoptab_list[STD_OUT] = &uart_out;
		devoptab_list[STD_ERR] = &uart_out;
	}
}
