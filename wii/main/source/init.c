#include "main.h"

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
    //alloc_init();
    __dsp_shutdown();
    if(!err) err = initExi();
    return err;
}
