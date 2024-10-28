#include "main.h"

Sys::DiscDrive::DiscDrive() {
    printf("DI_Init...\r\n");
    DI_Init();
    printf("DI_Mount...\r\n");
    DI_Mount();
}

int Sys::DiscDrive::getStatus() {
    return DI_GetStatus();
}

int Sys::DiscDrive::getDiscId(u64 &id) {
    int stat = DI_GetStatus();
    if(stat & DVD_NO_DISC) return -1; //-ENOMEDIUM;
    if(!(stat & DVD_READY)) return -EAGAIN;
    DI_ReadDiscID(&id);
    return 0;
}
