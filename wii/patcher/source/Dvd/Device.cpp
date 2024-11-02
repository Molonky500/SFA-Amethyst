#include "main.h"
#include "Dvd/Device.h"
#include "Dvd/File.h"
#include "GcIso/Iso.h"

//raw DVD register access because libogc doesn't work.
typedef struct {
    volatile u32 SR;      //status register
    volatile u32 CVR;     //cover register (status2)
    volatile u32 CMDBUF0; //command buffer 0
    volatile u32 CMDBUF1; //command buffer 1
    volatile u32 CMDBUF2; //command buffer 2
    volatile u32 MAR;     //DMA memory address register
    volatile u32 LENGTH;  //DMA transfer length register
    volatile u32 CR;      //control register
    volatile u32 IMMBUF;  //immediate data buffer
    volatile u32 CFG;     //configuration register
} DI_Struct;
DI_Struct *DI = (DI_Struct*)0xCD806000;

#define DISR_BRKINT (1 << 6)
#define DISR_BRKINTMASK (1 << 5)
#define DISR_TCINT (1 << 4)
#define DISR_TCINTMASK (1 << 3)
#define DISR_DEINT (1 << 2)
#define DISR_DEINTMASK (1 << 1)
#define DISR_BRK (1 << 0)

#define DICVR_CVRINT (1 << 2)
#define DICVR_CVRINTMASK (1 << 1)
#define DICVR_CVR (1 << 0)

#define DICR_RW (1 << 2)
#define DICR_DMA (1 << 1)
#define DICR_TSTART (1 << 0)

#define DICMD_INQUIRY    0x12000000
#define DICMD_READ       0xA8000000
#define DICMD_IDENTIFY   0xA8000040 //read disc ID/init drive
#define DICMD_SEEK       0xAB000000
#define DICMD_READ_DVD   0xD0000000 //for video DVDs/DVDRs
#define DICMD_GET_ERROR  0xE0000000
#define DICMD_STOP_MOTOR 0xE3000000

//DVD error codes high byte
#define DVD_ERR_LID_OPEN     0x01000000
#define DVD_ERR_DISC_CHANGED 0x02000000
#define DVD_ERR_NO_DISC      0x03000000
#define DVD_ERR_MOTOR_OFF    0x04000000
#define DVD_ERR_NOT_INIT     0x05000000 //disc ID not read

auto HW_GPIO_OUT = (volatile u32*)0xCD8000E0;
auto HW_RESETS   = (volatile u32*)0xCD800194;

static void _dvd_reset() {
    *HW_GPIO_OUT &= ~0x10; //enable spinup on reset
    *HW_RESETS &= ~0x400; //pull DI reset low (assert)
	usleep(1000);
    *HW_RESETS |= 0x400; //pull DI reset high (deassert)
}

static void _dvd_writeCmd(u32 cmd0, u32 cmd1, u32 cmd2) {
    DI->CMDBUF0 = cmd0;
    DI->CMDBUF1 = cmd1;
    DI->CMDBUF2 = cmd2;
}

static void _dvd_doXfer(u32 cr) {
    DI->CR = cr | DICR_TSTART;
    while(DI->CR & DICR_TSTART) LWP_YieldThread();
}

static void _dvd_identify() {
    u8 buf[2048] __attribute__((aligned(32)));
    _dvd_writeCmd(DICMD_IDENTIFY, 0, 0x20);
    DI->MAR = ((u32)buf) & 0x1FFFFFFF;
    DI->LENGTH = 0x20;
    _dvd_doXfer(DICR_DMA);
}

Sys::Dvd::Device::Device() {
    this->iso = nullptr;
    _dvd_reset();
    _dvd_identify();
    u32 err = this->getError();
	if(err) {
        printf("DVD init: error 0x%08X\r\n", err);
        return;
    }

    u64 id = 0xEEEEEEEEEEEEEEEELL;
    int r = this->getDiscId(id);
    printf("DVD init %d, ID = 0x%llX\r\n", r, id);

    Sys::Dvd::File::_initIoWrapper();
    //this->iso = new GcIso::Iso("dvdraw:");
    //this->iso->mount("dvd");
}

Sys::Dvd::Device::~Device() {
    if(this->iso) {
        this->iso->unmount();
        delete this->iso;
    }
}

u32 Sys::Dvd::Device::getError() {
    _dvd_writeCmd(DICMD_GET_ERROR, 0, 0);
	DI->IMMBUF = 0;
	_dvd_doXfer(0);
	return DI->IMMBUF;
}

int Sys::Dvd::Device::getStatus() {
    uint32_t cover = 0;
    int err = DI_GetCoverRegister(&cover);
    //printf("DI_GetCoverRegister => %d, %d\r\n", err, cover);
    if(err) return err;
    if(cover != DVD_COVER_DISC_INSERTED) return DVD_NO_DISC;

    //if there's any error, we need to reset.
    //buffers won't be updated, so we can get stale info.

    uint32_t status = 0;
    err = DI_GetError(&status);
    //printf("DI_GetError => %d, 0x%08X\r\n", err, status);
    if(err) return err;
    switch(status >> 24) {
        case 0: //Ready
        case 1: //ReadyNoReadsMade
            break;

        case 2: //CoverOpened
            //printf("Resetting DVD\r\n");
            DI_Reset();
            return DVD_NO_DISC;

        case 3: //DiscChangeDetected
            //printf("Resetting DVD\r\n");
            DI_Reset();
            return DVD_INIT;

        case 4: //NoMediumPresent
            return DVD_NO_DISC;

        case 5: //MotorStopped
        case 6: //DiscIdNotRead
        default:
            break;
    }
    return DI_GetStatus();
}

int Sys::Dvd::Device::getDiscId(u64 &id) {
    u8 buf[Sys::Dvd::SECTOR_SIZE] __attribute__((aligned(32)));
    //memset(buf, 0, sizeof(buf));
	this->read(buf, Sys::Dvd::SECTOR_SIZE, 0ULL);
	memcpy(&id, buf, 8);
	return 0;
}

int Sys::Dvd::Device::read(void *buf, uint32_t size, uint32_t offset) {
    //XXX is offset in sectors or bytes?
    static bool isDvdVideo = false;
	DCInvalidateRange(buf, size);
	DI->CVR = 0;
    _dvd_writeCmd(
        isDvdVideo ? DICMD_READ_DVD : DICMD_READ,
        offset >> (isDvdVideo ? 11 : 2),
        size >> (isDvdVideo ? 11 : 0));
	DI->MAR = ((u32)buf) & 0x1FFFFFFF;
	DI->LENGTH = size;
	_dvd_doXfer(DICR_DMA);
	if(DI->SR & DISR_DEINT) {
        printf("DVD read error (0x%08X)\r\n", DI->SR);
		return -EIO;
    }
	return size;
}
