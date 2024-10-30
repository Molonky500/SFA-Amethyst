#include "main.h"
#include "Dvd/Device.h"

Sys::Dvd::Device() {
    printf("DI_Init...\r\n");
    DI_Init();
    printf("DI_Mount...\r\n");
    DI_Mount();
    Sys::Dvd::File::_initIoWrapper();
    //XXX read FST
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
    int stat = DI_GetStatus();
    if(stat & DVD_NO_DISC) return -1; //-ENOMEDIUM;
    if(!(stat & DVD_READY)) return -EAGAIN;
    return DI_ReadDiscID(&id);
}

int Sys::Dvd::Device::read(void *buf, uint32_t size, uint32_t offset) {
    //XXX is offset in sectors or bytes?
    this->getStatus(); //resets if needed (XXX rename)
    while(true) {
        int r = DI_Read(void *buf, uint32_t size, uint32_t offset);
        if(r == -EAGAIN) sleep(1);
        else return r;
    }
}
