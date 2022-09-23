#include "main.h"

static int initFilesystem() {
    /** Init FAT driver.
     *  @returns 0 on success, nonzero on failure.
     */
    if(!fatInitDefault()) {
        printf("FAT init failed\n");
        return 1;
    }
    return 0;
}

static int initExi() {
    exiPrintInit();
    return 0;
}

extern void __dsp_shutdown();

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    int err = 0;
    alloc_init();
    SYS_ProtectRange(SYS_PROTECTCHAN0,
        (void*)0x90000000, 0x01000000, SYS_PROTECTNONE);
    __dsp_shutdown();
    if(!err) err = initExi();
    if(!err) err = initFilesystem();
    return err;
}
