#include "main.h"

/** EXI debug test program.
 */

__attribute__ ((aligned (32))) static u8 dmaBuf[DMA_BUF_SIZE];
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
bool gIsVideoInit = false;
vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;
s8  chan   = 1;
s8  cs     = 1;
s8  speed  = 0;
u8  payload[4] = {0x70, 0x00, 0x00, 0x00};
s32 dataMode = 0;
s32 testMode = TEST_BOTH;
u8 debugPort = 0;
bool autoWrite = false;
bool useDma = false;
bool useBulk = false;
u32  writeCount = 0;
u32  lastRx = 0xD06FECE5;
static const char *statusMsg = "";

vu8 rxBuf[RX_BUF_SIZE];
volatile int rxPos = 0;

uint32_t crc32b(const void *data_, uint32_t len) {
    const uint8_t *data = (const uint8_t*)data_;
    unsigned int byte, crc, mask;

    crc = 0xFFFFFFFF;
    for(uint32_t i=0; i<len; i++) {
        crc = crc ^ data[i];
        for(uint32_t j = 0; j < 8; j++) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return crc; //should be ~crc, but we save a few cycles by not doing that
}

int initVideo() {
    /** Init video system.
     *  @returns 0 on success, nonzero on failure.
     */
	VIDEO_Init(); //Initialise the video system

    // Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode); //set up selected video mode
	VIDEO_SetNextFramebuffer(xfb); //tell GPU where our buffers are
	VIDEO_SetBlack(FALSE); //main screen turn on
    VIDEO_Flush();
    VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    gIsVideoInit = true;

    return 0;
}

void quit(const char *msg) {
    printf("\n   ***   Exiting  ***  \n");
    exit(0);
}

vu32 exi1cnt = 0, ext1cnt = 0, tc1cnt = 0;
void isrExi1Exi(u32 irq, void *ctx) {
    static u32 iChan = 1;
    rxBuf[rxPos++] = _exiReg[(iChan*5)+4];
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;
    exi1cnt++;
    _exiReg[(iChan*5)+0] |= (1 << 1); //clear interrupt
}
void isrExi1Ext(u32 irq, void *ctx) {
    static u32 iChan = 1;
    _exiReg[(iChan*5)+0] |= (1 << 11); //clear interrupt
    ext1cnt++;
}
void isrExi1Tc(u32 irq, void *ctx) {
    static u32 iChan = 1;
    _exiReg[(iChan*5)+0] |= (1 << 3); //clear interrupt
    tc1cnt++;
}

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    int err = 0;
    initVideo();
    PAD_Init();
    return err;
}

void waitTransaction() {
    volatile u32 *exi = (volatile u32*)(0xCD006800 + (chan * 0x14));
    while(exi[3] & 1); //wait for any previous transfer
}

static u32 prev0;
volatile u32* beginTransaction() {
    waitTransaction();
    volatile u32 *exi = (volatile u32*)(0xCD006800 + (chan * 0x14));
    exi[0] =
        //(1 <<  0) | //enable interrupt
        //(1 << 10) | //enable interrupt
        (1 << 13) | //ROMDIS
        //(3 << 4) | //8MHz
        //(0 << 4) | //1MHz
        (speed << 4) |
        (cs << 7);
        //(1 << 7); //device 0 (Gecko)
        //(2 << 7); //device 1 (UART)
    return exi;
}

void endTransaction() {
    volatile u32 *exi = (volatile u32*)(0xCD006800 + (chan * 0x14));
    exi[0] = prev0;
}

void readWrite8(volatile u32 *exi, u8 val) {
    //exi[4] = val;
    u32 v = val;
    exi[4] = (v << 24) | (v << 16) | (v << 8) | v;
    exi[3] = (2 << 2) | //read and write
        (0 << 4) | //1 byte
        (1 << 0); //start now
    waitTransaction();

    lastRx = exi[4];
    rxBuf[rxPos++] = lastRx & 0xFF; //XXX should this be >> 24 ?
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;
}

void readWrite16(volatile u32 *exi, u16 val) {
    exi[4] = ((u32)val) << 16;
    exi[3] = (2 << 2) | //read and write
        (1 << 4) | //2 bytes
        (1 << 0); //start now
    waitTransaction();

    lastRx = exi[4];
    rxBuf[rxPos++] = (lastRx >> 24) & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;

    rxBuf[rxPos++] = (lastRx >> 16) & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;
}

void readWrite32(volatile u32 *exi, u32 val) {
    exi[4] = val;
    exi[3] = (2 << 2) | //read and write
        (3 << 4) | //4 bytes
        (1 << 0); //start now
    waitTransaction();

    lastRx = exi[4];
    //work around bug that causes first bit to be missed
    //with custom gecko device
    if(!(lastRx & 1)) lastRx <<= 1;

    rxBuf[rxPos++] = (lastRx >> 24) & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;

    rxBuf[rxPos++] = (lastRx >> 16) & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;

    rxBuf[rxPos++] = (lastRx >> 8) & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;

    rxBuf[rxPos++] = lastRx & 0xFF;
    if(rxPos >= RX_BUF_SIZE) rxPos = 0;
}

void writeDma(volatile u32 *exi, const void *msg, u32 len) {
    waitTransaction();
    u32 remLen = len;
    for(u32 i=0; i<len; i += DMA_BUF_SIZE) {
        u32 clen = MIN(DMA_BUF_SIZE, remLen);

        //DMA length must be multiple of 32.
        //that means we send some extra zeros, but they'll be
        //interpreted as NOP commands, ie ignored.
        u32 tlen = clen;
        u32 pad  = tlen & 0x1F;
        if(pad) tlen += 32 - pad;

        //memset(&dmaBuf[clen], 0, tlen-clen);
        memset(dmaBuf, 0, DMA_BUF_SIZE);
        memcpy(dmaBuf, msg, clen);
        DCFlushRange(dmaBuf, DMA_BUF_SIZE);
        exi[1] = MEM_VIRTUAL_TO_PHYSICAL(dmaBuf); //DMA source
        exi[2] = tlen; //DMA length
        exi[3] = (1 << 2) | //write
            (1 << 1) | //use DMA
            (1 << 0); //start now

        if(remLen < DMA_BUF_SIZE) break;
        remLen -= DMA_BUF_SIZE;
        msg += DMA_BUF_SIZE;
        waitTransaction();
    }
}

void bulkWriteString(const void *msg, u32 len) {
    const u32 *data = (const u32*)msg;

    u32 crc = crc32b(msg, len);
    volatile u32 *exi = beginTransaction();
    //readWrite32(exi, 0x41000000 | len); //bulk send with CRC
    readWrite32(exi, 0x40000000 | len); //bulk send without CRC
    if(useDma) {
        writeDma(exi, msg, len);
        readWrite32(exi, 0); //inexplicably required or else DMA will
            //only send the first word!?
    }
    else {
        for(u32 i=0; i<len; i += 4) {
            readWrite32(exi, data[i>>2]);
            //udelay(100);
        }
    }
    //readWrite32(exi, crc);
    endTransaction();
}

void writeString(const void *msg, u32 len) {
    if(useBulk) {
        bulkWriteString(msg, len);
        return;
    }

    const u8 *data = (const u8*)msg;
    volatile u32 *exi = beginTransaction();
    u32 i = 0;
    while(i < len) {
        u32 out = 0, cnt = 0;
        while(++cnt < 3) {
            out = (out << 8) | (u32)(data[i++]);
            if(i >= len) break;
        }
        //if(cnt == 2) out <<= 8;
        //if(cnt == 1) out <<= 16;
        readWrite32(exi, (cnt << 28) | out);
        waitTransaction();
    }
    endTransaction();
}

void doWrite() {
    volatile u32 *exi;
    switch(dataMode) {
        case HELLO:
            exi = beginTransaction();
            readWrite32(exi, 0x01230ABC);
            endTransaction();
            break;
        case TEXT: {
            char msg[256];
            sprintf(msg, "Message %d of a zillion. %d is a random number I pulled from my arse. %d is another one.\r\n",
                writeCount, rand(), rand());
            writeString(msg, strlen(msg));
            break;
        }
        case PAYLOAD:
            exi = beginTransaction();
            readWrite32(exi,
                (payload[0] << 24) | (payload[1] << 16) |
                (payload[2] <<  8) |  payload[3]);
            endTransaction();
            break;
        case ZERO:
            exi = beginTransaction();
            readWrite32(exi, 0);
            endTransaction();
            break;
        case ONE:
            exi = beginTransaction();
            readWrite32(exi, 0xFFFFFFFF);
            endTransaction();
            break;
        case ZERO_ONE:
            exi = beginTransaction();
            readWrite32(exi, 0x55555555);
            endTransaction();
            break;
        case ONE_ZERO:
            exi = beginTransaction();
            readWrite32(exi, 0xAAAAAAAA);
            endTransaction();
            break;
        case RANDOM:
            exi = beginTransaction();
            readWrite32(exi, rand());
            endTransaction();
            break;
    }
    writeCount++;
}

void doRead() {
    volatile u32 *exi = beginTransaction();
    if(useBulk) {
        readWrite32(exi, 0x60000008);
        readWrite32(exi, 0x00000000);
        readWrite32(exi, 0x00000000);
    }
    else {
        int limit = 100;
        do {
            if(!(limit--)) break;
            readWrite32(exi, 0x50000000);
        } while(lastRx & (1 << 26));
    }
}

void doExec() {
    if(testMode != TEST_READ) doWrite();
    if(testMode != TEST_WRITE) doRead();
}

void drawLastRx() {
    int idx = rxPos;
    for(int row=0; row<RX_BUF_SIZE/16; row++) {
        printf("%04X ", row * 16);

        char chrs[17];
        for(int col=0; col<16; col++) {
            u8 b = rxBuf[idx++];
            if(idx >= RX_BUF_SIZE) idx = 0;
            printf("%02X%s", b, ((col&3)==3) ? "  " : " ");

            if(b < 0x20 || b > 0x7E) b = 0x2E;
            chrs[col] = b;
            chrs[col+1] = 0;
        }
        for(int col=0; col<16; col++) {
            printf("%c%s", chrs[col], ((col&3)==3) ? " " : "");
        }
        //printf("\n"); //the line is already the exact width needed
    }
}

int main(int argc, char **argv) {
    _ipcReg[0x64>>2]  = 0xFFFFFFFF; //AHBPROT: access everything
    _ipcReg[0xFC>>2] |= 0x00FF0020; //allow PPC access
    _ipcReg[0xDC>>2]  = 0xFFFFFFFF; //enable GPIOs
    _ipcReg[0xC4>>2] |= 0x00FF0020; //set directions
    SET_DISC_LED(1);
    int err = init();
    if(err) quit("Startup failed\n");

    IRQ_Request(IRQ_EXI1_EXI, isrExi1Exi, NULL);
    IRQ_Request(IRQ_EXI1_EXT, isrExi1Ext, NULL);
    IRQ_Request(IRQ_EXI1_TC,  isrExi1Tc,  NULL);
    __UnmaskIrq(IM_EXI1);
    _exiReg[(1*5)+0] |= (1 << 0) | //enable EXI interrupt
        (1 << 10) | //enable EXT interrupt
        (1 << 2); //enable TC interrupt
    SET_DISC_LED(0);

    while(1) {
        VIDEO_WaitVSync();
        PAD_ScanPads();

        volatile u32 *exi = (volatile u32*)0xCD006800;

        static const char *spinner = "/-\\|";
        static uint32_t spinnerTick = 0;
        printf(
            "\x1B[1;1H\x1B[2K" //home, clear line
            "%c START:Exit  Z:Faster  "
            "<>:+/- 0x1  XY:+/- 0x10  LR:+/- 0x100\n",
                spinner[(spinnerTick >> 2) & 3]);
        spinnerTick++;
        drawMenu();
        runMenu();
        drawLastRx();

        printf("\x1B[2;25H%s ", statusMsg);
        printf(
            "\x1B[3;40HABC"
            "\x1B[4;40H%c%c%c Conn"
            "\x1B[5;40H%c%c%c Int"
            "\x1B[6;40H%c%c%c TC"
            "\x1B[7;40H%c%c%c EXT"
            "\x1B[8;40HWriteCnt: %9d",
                (exi[ 0] & (1 << 12)) ? '*' : ' ',
                (exi[ 5] & (1 << 12)) ? '*' : ' ',
                (exi[10] & (1 << 12)) ? '*' : ' ',
                (exi[ 0] & (1 <<  1)) ? '*' : ' ',
                (exi[ 5] & (1 <<  1)) ? '*' : ' ',
                (exi[10] & (1 <<  1)) ? '*' : ' ',
                (exi[ 0] & (1 <<  3)) ? '*' : ' ',
                (exi[ 5] & (1 <<  3)) ? '*' : ' ',
                (exi[10] & (1 <<  3)) ? '*' : ' ',
                (exi[ 0] & (1 << 11)) ? '*' : ' ',
                (exi[ 5] & (1 << 11)) ? '*' : ' ',
                (exi[10] & (1 << 11)) ? '*' : ' ',
                writeCount);

        u32 bHeld = PAD_ButtonsHeld(PAD_CHAN0);
        if(autoWrite || (bHeld & PAD_BUTTON_B)) doExec();
    }
	return 0;
}
