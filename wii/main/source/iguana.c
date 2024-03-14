#include "main.h"

static u16 sendSpeedFlags=0, recvSpeedFlags=0; //default to 1MHz
static u16 bulkSendSpeedFlags=0, bulkRecvSpeedFlags=0;
static u32 iguanaProtoVer = 0;
static u32 lastHwctl = 0x70000001; //HWCTL msg, INT enabled
static volatile u32 *exi = (volatile u32*)0xCD006814; //channel 1

void iguanaPuts(const char *str) {
    if(debugDeviceType != DBG_DEV_IGUANA) return;
    u32 irq = IRQ_Disable();
    ssize_t len;
    if(str != exiDmaBuf) {
        memset(exiDmaBuf, 0, sizeof(exiDmaBuf));
        len = fixCrlf(str, exiDmaBuf, sizeof(exiDmaBuf));
    }
    else {
        char buf[1024];
        len = fixCrlf(str, buf, sizeof(buf));
        memcpy(exiDmaBuf, buf, len+1); //+1 for null
        str = (const char*)exiDmaBuf;
    }
    DCFlushRange((char*)str, len);

    //pad the length - must be a multiple of 32 bytes
    ssize_t paddedLen = len;
    ssize_t pad = paddedLen & 0x1F;
    if(pad) paddedLen += (32-pad);

    while(exi[3] & 1); //wait for any previous transfer
    u32 prev0 = exi[0];
    //u32 prev1 = exi[1];
    //u32 prev2 = exi[2];
    exi[0] = bulkSendSpeedFlags |
        (1 << 13) | //ROMDIS
        (1 <<  7) | //device 0 (Gecko)
        (1 <<  0);  //enable INT interrupt

    exi[4] = 0x40000000 | len; //Iguana SENDBULK cmd
    exi[3] = (1 << 2) | //write
        (3 << 4) | //4 bytes
        (1 << 0); //start now
    while(exi[3] & 1); //wait for transfer

    exi[1] = MEM_VIRTUAL_TO_PHYSICAL(exiDmaBuf); //DMA source
    exi[2] = paddedLen; //DMA length
    exi[3] = (1 << 2) | //write
        (1 << 1) | //use DMA
        (1 << 0); //start now
    while(exi[3] & 1); //wait for transfer

    //exi[0] |= (1 << 1) | (1 << 11); //clear interrupt flags
    //exi[2] = prev2;
    //exi[1] = prev1;
    exi[0] = prev0;
    IRQ_Restore(irq);
}

void iguanaPutsNoFlush(const char *str) {
    //write string using IMM transfers and no cache flushes
    if(debugDeviceType != DBG_DEV_IGUANA) return;
    u32 irq = IRQ_Disable();
    char buf[1024];
    size_t len = fixCrlf(str, buf, sizeof(buf));

    while(exi[3] & 1); //wait for any previous transfer
    u32 prev0 = exi[0];
    //u32 prev1 = exi[1];
    //u32 prev2 = exi[2];
    exi[0] = bulkSendSpeedFlags |
        (1 << 13) | //ROMDIS
        (1 <<  7) | //device 0 (Gecko)
        (1 <<  0);  //enable INT interrupt

    exi[4] = 0x40000000 | len; //Iguana SENDBULK cmd
    exi[3] = (1 << 2) | //write
        (3 << 4) | //4 bytes
        (1 << 0); //start now
    while(exi[3] & 1); //wait for transfer

    for(int i=0; i<len; i += 4) {
        u32 data = 0;
        for(int j=0; j<4; j++) {
            data <<= 8;
            if(i+j < len) data |= buf[i+j];
        }
        exi[4] = data;
        exi[3] = (1 << 2) | //write
            (3 << 4) | //4 bytes
            (1 << 0); //start now
        while(exi[3] & 1); //wait for transfer
        udelay(500); //XXX properly check if transfer succeeded
    }

    //exi[0] |= (1 << 1) | (1 << 11); //clear interrupt flags
    //exi[2] = prev2;
    //exi[1] = prev1;
    exi[0] = prev0;
    IRQ_Restore(irq);
}

static u32 _iguanaReadWrite32(u32 val) {
    u32 res = exiReadWrite32(exi, val);
    if(!(res & 1)) res <<= 1; //work around SPI clock bug
    return res;
}

u32 iguanaReadWrite(u32 val) {
    if(debugDeviceType != DBG_DEV_IGUANA) return 0xFFFFFFFF;
    u32 res = _iguanaReadWrite32(val);
    for(int tries=0; tries<100; tries++) {
        if(res & 0xF0000000) break;
        res = _iguanaReadWrite32(0);
    }
    return res;
}

void iguanaSetHwctl(int bit, bool on) {
    if(debugDeviceType != DBG_DEV_IGUANA) return;
    if(on) lastHwctl |=  (1 << bit);
    else   lastHwctl &= ~(1 << bit);
    iguanaReadWrite(lastHwctl);
}
void iguanaSetRedLed(bool on) {
    iguanaSetHwctl(3, on);
}
void iguanaSetGreenLed(bool on) {
    iguanaSetHwctl(2, on);
}
void iguanaSetBlueLed(bool on) {
    iguanaSetHwctl(1, on);
}

void iguanaInit() {
    debugDeviceType = DBG_DEV_GECKO; //XXX test for Gecko
    exi[0] =
        (1 << 13) | //ROMDIS
        (1 << 10) | //enable EXT interrupt
        (0 <<  4) | //1MHz
        (1 <<  7) | //device 0 (Gecko)
        (1 <<  0);  //enable EXI interrupt
    for(int tries=0; tries<10; tries++) {
        u32 resp = exiReadWrite32(exi, 0x01230ABC);
        if(resp == 0x90000001) {
            debugDeviceType = DBG_DEV_IGUANA;
            break;
        }
        /*else if(resp & 0xFF000000) {
            debugDeviceType = DBG_DEV_NONE;
            return;
        }*/
        udelay(10000);
    }

    //query protocol version
    iguanaProtoVer = iguanaReadWrite(0x80000000); //QUERY PROTO_VERSION

    //query supported speeds
    u32 res = iguanaReadWrite(0x82000000) >> 2; //QUERY SPEED
    sendSpeedFlags     = ((res >> 19) & 7) << 13; //shift to EXI CR speed bits
    recvSpeedFlags     = ((res >> 16) & 7) << 13;
    bulkSendSpeedFlags = ((res >> 11) & 7) << 13;
    bulkRecvSpeedFlags = ((res >>  8) & 7) << 13;

    //enable INT line
    iguanaReadWrite(lastHwctl); //HWCTL
    __UnmaskIrq(IM_EXI1);
}
