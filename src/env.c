#include "main.h"

static void (*getEnvfxAct_replaced)(void *buf, DataFileEnum32 fileId, uint idx, int size);
static void (*getEnvfxActImm_replaced)(void *buf, DataFileEnum32 fileId, uint idx, int size);

void getEnvFxActHook(void *buf, DataFileEnum32 fileId, uint idx, int size) {
    DPRINT("GetEnvFxAct(0x%04X)\n", idx / 0x60);
    getTabEntry(buf, fileId, idx, size);
}

void envHooksInit() {
    getEnvfxAct_replaced =
        (void(*)(void *buf, DataFileEnum32 fileId, uint idx, int size))
        hookBranch(0x80008cfc, getEnvFxActHook, 1);
    getEnvfxActImm_replaced =
        (void(*)(void *buf, DataFileEnum32 fileId, uint idx, int size))
        hookBranch(0x80008bb4, getEnvFxActHook, 1);

    //let it rain in Cape Claw sometimes
    //XXX ensure the rain isn't disabled when we enter. (or don't?
    //that just means it'll be foggy but not rainy sometimes.)
    //also this is iffy because the rain will appear/disappear as we
    //walk between CC and the link map.
    WRITE16(0x803235F0, 0x01A8);
    WRITE16(0x80323600, 0x008A);
    WRITE16(0x80323580, 0x01B8);
    WRITE16(0x80323590, 0x01B8);
    WRITE16(0x80323548, 0x01B9);
    WRITE16(0x80323558, 0x01B9);
    WRITE16(0x803235B8, 0x01BA);
    WRITE16(0x803235C8, 0x01BA);

    //make it snow more in SnowHorn Wastes sometimes
    WRITE16(0x80326af8, 0x002A);
    WRITE16(0x80326b0c, 0x0102);
    WRITE16(0x80326b26, 0x002A);
    //for other maps there's no weather table so we'd have to add one...
    //(TTH has one but it's fine as it is)
}
