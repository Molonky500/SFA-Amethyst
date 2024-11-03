#include "main.h"
#include "Dvd/Device.h"
#include "Dvd/File.h"
#include "GcIso/Iso.h"

DI_Struct *DI = (DI_Struct*)0xCD806000;

auto HW_GPIO_OUT = (volatile u32*)0xCD8000E0;
auto HW_RESETS   = (volatile u32*)0xCD800194;

void Sys::Dvd::Device::_reset() {
    *HW_GPIO_OUT &= ~0x10; //enable spinup on reset
    *HW_RESETS &= ~0x400; //pull DI reset low (assert)
	usleep(1000);
    *HW_RESETS |= 0x400; //pull DI reset high (deassert)
}

void Sys::Dvd::Device::_writeCmd(u32 cmd0, u32 cmd1, u32 cmd2) {
    DI->CMDBUF0 = cmd0;
    DI->CMDBUF1 = cmd1;
    DI->CMDBUF2 = cmd2;
}

void Sys::Dvd::Device::_doXfer(u32 cr) {
    DI->CR = cr | DICR_TSTART;
    while(DI->CR & DICR_TSTART) LWP_YieldThread();
}

void Sys::Dvd::Device::_identify() {
    u8 buf[2048] __attribute__((aligned(32)));
    this->_writeCmd(DICMD_IDENTIFY, 0, 0x20);
    DI->MAR = ((u32)buf) & 0x1FFFFFFF;
    DI->LENGTH = 0x20;
    this->_doXfer(DICR_DMA);
}

Sys::Dvd::Device::Device() {
    this->iso = nullptr;
    this->_reset();
    this->_identify();
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
    this->_writeCmd(DICMD_GET_ERROR, 0, 0);
	DI->IMMBUF = 0;
	this->_doXfer(0);
	return DI->IMMBUF;
}

int Sys::Dvd::Device::getStatus() {
    uint32_t cover = DI->CVR;
    if(cover & DICVR_CVR) return DVD_NO_DISC;

    //if there's any error, we need to reset.
    //buffers won't be updated, so we can get stale info.

    uint32_t err = this->getError();
    printf("DI Error = 0x%08X\r\n", err);
    switch(err >> 24) {
        case 0: //Ready
        case 1: //ReadyNoReadsMade
            break;

        case 2: //CoverOpened
            //printf("Resetting DVD\r\n");
            this->_reset();
            return DVD_NO_DISC;

        case 3: //DiscChangeDetected
            //printf("Resetting DVD\r\n");
            this->_reset();
            return DVD_INIT;

        case 4: //NoMediumPresent
            return DVD_NO_DISC;

        case 5: //MotorStopped
        case 6: //DiscIdNotRead
        default:
            break;
    }
    return DI->SR;
}

int Sys::Dvd::Device::getDiscId(u64 &id) {
    u8 buf[Sys::Dvd::SECTOR_SIZE] __attribute__((aligned(32)));
    //memset(buf, 0, sizeof(buf));
	this->read(buf, Sys::Dvd::SECTOR_SIZE, 0ULL);
	memcpy(&id, buf, 8);
	return 0;
}

int Sys::Dvd::Device::_read(void *dst, uint32_t size, uint32_t offset,
bool isDvdVideo) {
    DCInvalidateRange(dst, size);
    DI->CVR = 0;
    this->_writeCmd(
        isDvdVideo ? DICMD_READ_DVD : DICMD_READ,
        offset >> (isDvdVideo ? 11 : 2),
        size >> (isDvdVideo ? 11 : 0));
    DI->MAR = ((u32)dst) & 0x1FFFFFFF;
    DI->LENGTH = size;
    //clear interrupt flags
    DI->SR |= DISR_DEINT | DISR_TCINT | DISR_BRKINT;
    this->_doXfer(DICR_DMA);
    u32 sr = DI->SR;
    if(sr & DISR_DEINT) {
        fprintf(stderr, "DVD read error (SR=0x%08X, code 0x%08X)\r\n", sr,
            this->getError());
        return -EIO;
    }
    return size;
}

int Sys::Dvd::Device::read(void *dst, uint32_t size, uint32_t offset) {
    u8 buf[Sys::Dvd::SECTOR_SIZE] __attribute__((aligned(32)));
    static bool isDvdVideo = false;

    //abstract away the minimum size/offset requirements.

    uint32_t blockSize = 1 << (isDvdVideo ? 11 : 2);
    uint32_t offsMask  = blockSize - 1;
    uint32_t offsAlign = offset & offsMask;
    offsMask = ~offsMask;
    if(offsAlign) { //offset is partway through a block
        DCInvalidateRange(buf, blockSize);
        int rLen = this->_read(buf, Sys::Dvd::SECTOR_SIZE,
            offset & offsMask, isDvdVideo);
        printf("Read partial block 0x%X (0x%X) len %d => %d\r\n", offset,
            offset & offsMask, blockSize, rLen);
        if(rLen <= 0) return rLen;
        memcpy(dst, &buf[offsAlign], rLen - offsAlign);
        offset += rLen - offsAlign;
        dst += rLen - offsAlign;
    }

    if((!(((u32)dst) & ~31)) //destination is aligned
    && !(size & (Sys::Dvd::SECTOR_SIZE-1))) { //size is aligned
        int rLen = this->_read(dst, size, offset, isDvdVideo);
        printf("Read aligned 0x%X len %d => %d\r\n", offset, size, rLen);
        return rLen;
    }

    //destination isn't aligned, so do it the slow way
    uint32_t nRead = 0;
    while(size > 0) {
        uint32_t xferLen = MIN(size, Sys::Dvd::SECTOR_SIZE);
        DCInvalidateRange(buf, xferLen);
        int rLen = this->_read(buf, Sys::Dvd::SECTOR_SIZE,
            offset, isDvdVideo);
        printf("Read unaligned 0x%X size %d => %d\r\n",
            offset, xferLen, rLen);
        if(rLen <= 0) return rLen;
        nRead += rLen;
        memcpy(dst, buf, rLen);
        dst += rLen;
        size -= rLen;
    }
	return nRead;
}
