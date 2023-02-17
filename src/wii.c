#include "main.h"

void wiiHooksInit() {
    //for now we'll still use DVD...

    //enable audio streaming (not sure why it's disabled,
    //probably Wii ignores the streaming flag in header)
    //must be done before any disc access.
    /*vu32 *dvd = (vu32*)0xCD006000;
    dvd[2] = 0xE4010000; //enable audio (DICMDBUF0)
    dvd[3] = 0; //DICMDBUF1
    dvd[4] = 0; //DICMDBUF
    dvd[5] = 0; //DIMAR
    dvd[6] = 0; //DILENGTH
    dvd[7] = 3; //DICR */

    //WRITE_NOP(0x80249288); //let it think it's booted from JTAG
    //    //so it will load FST.
    _DVDInit();

    //don't try to init DVD again...
    //WRITE_NOP(0x80020db0); //...in game init()
    //WRITE_NOP(0x80244600); //...in __OSReboot()

    //__ARCheckSize probes ARAM to detect size, but this
    //clobbers MEM2, so disable it and set the size
    //manually instead.
    //WRITE_BLR(0x80250218); //__ARCheckSize
    //WRITE32(0x803de01c, 0x01000000); //__ARSize

    wiiControlsInit();
}
