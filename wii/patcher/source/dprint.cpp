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

void waitTransaction() {
    volatile u32 *exi = (volatile u32*)(0xCD006800 + (chan * 0x14));
    while(exi[3] & 1); //wait for any previous transfer
}

volatile u32* beginTransaction() {
    waitTransaction();
    volatile u32 *exi = (volatile u32*)(0xCD006800 + (chan * 0x14));
    exi[0] =
        //(1 <<  0) | //enable interrupt
        //(1 << 10) | //enable interrupt
        (1 << 13) | //ROMDIS
        (3 << 4) | //8MHz
        //(0 << 4) | //1MHz
        //(1 << 7); //device 0 (Gecko)
        (2 << 7); //device 1 (UART)
    return exi;
}

static ssize_t __uart_write(struct _reent *r, void* fd,
const char *ptr, size_t len) {
	u32 level;
	LWP_MutexLock(gecko_mutex);
	level = IRQ_Disable();

	volatile u32 *exi = beginTransaction();
	//for UART we need to send address first.
	//0x800400 is address for UART.
	//high bit indicates write.
	static u32 addr = (0x800400 << 6) | 0x80000000;
	exi[4] = addr;
	exi[3] = (1 << 2) | //write
		(3 << 4) | //4 bytes
		(1 << 0); //start now
	waitTransaction();

	//then send message
	for(size_t i=0; i<len; i++) {
		exi[4] = ptr[i] << 24;
		exi[3] = (1 << 2) | //write
			(0 << 4) | //1 byte
			(1 << 0); //start now
		waitTransaction();
	}

	IRQ_Restore(level);
	LWP_MutexUnlock(gecko_mutex);
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
	if(sysType & 0xF0000000) gecko = false;
	else gecko = usb_isgeckoalive(1);
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
