#include "main.h"

/** EXI debug test program.
 */

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
bool gIsVideoInit = false;
vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD000000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;
u8  cs    = 0;
u8  speed = 0;

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
    printf("\n  Exiting  \n");
    exit(0);
}

vu32 exi1cnt = 0, ext1cnt = 0, tc1cnt = 0;
vu32 rxBuf[SPI_RXBUF_SIZE];
vu32 rxHead=0, rxTail=0;
void isrExi1Exi(u32 irq, void *ctx) {
    static u32 iChan = 1;
    rxBuf[rxHead] = _exiReg[(iChan*5)+4];
    rxHead = (rxHead+1) & (SPI_RXBUF_SIZE-1);
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

__attribute__ ((aligned (32))) static u8 dmaBuf[4096];
static u32 length = 32;
static u32 msgCnt = 0;

void doUartTest(const char *str) {
    //0x800400 is address for UART.
    //high bit indicates write.
    static u32 addr = (0x800400 << 6) | 0x80000000;
    memset(dmaBuf, 0, sizeof(dmaBuf));
    u8 *d = dmaBuf;
    //for UART we need to send address first.
    // *(d++) = (addr >> 24) & 0xFF;
    // *(d++) = (addr >> 16) & 0xFF;
    // *(d++) = (addr >>  8) & 0xFF;
    // *(d++) =  addr        & 0xFF,
    //for our weird Gecko-like thing we send length first.
    *(d++) = 0xDE; *(d++) = 0xAD; //magic
    *(d++) = 0xFA; *(d++) = 0xCE; //magic

    char msg[1024];
    if(!str) {
        snprintf(msg, sizeof(msg),
            "Message %d of a zillion. %d is a random number. %d is another one.\r\n",
            msgCnt++, rand(), rand());
        str = msg;
    }
    length = strlen(str) + 7;

    //DMA length must be multiple of 32.
    //that means we send some extra zeros, but the magic
    //header and length prefix mean the recipient knows
    //to just ignore it.
    u32 pad = length & 0x1F;
    if(pad) length += 32 - pad;
    length >>= 5; //since it has to be multiple of 32, may
    //as well encode "number of 32-byte chunks" rather than
    //"number of bytes"

    *(d++) = (length >>  8) & 0xFF;
    *(d++) = (length >>  0) & 0xFF;
    strcpy(d, str);
    DCFlushRange(dmaBuf, length << 5);
    static volatile u32 *exi =
        //(volatile u32*)0xCD006800; //channel 0 (actual UART)
        (volatile u32*)0xCD006814; //channel 1 (Gecko)
    while(exi[3] & 1); //wait for any previous transfer
    u32 prev0 = exi[0];
    exi[0] =
        (1 <<  0) | //enable interrupt
        (1 << 10) | //enable interrupt
        (1 << 13) | //ROMDIS
        //(3 << 4) | //8MHz
        //(0 << 4) | //1MHz
        (speed << 4) |
        (cs << 7);
        //(1 << 7); //device 0 (Gecko)
        //(2 << 7); //device 1 (UART)
    //printf("%08X\n", exi[0]);
    exi[1] = MEM_VIRTUAL_TO_PHYSICAL(dmaBuf); //DMA source
    //send one more block than needed so that the device has a
    //chance to read in all of the message, even if it has to
    //skip past a few padding bytes first.
    exi[2] = (length+1) << 5; //DMA length
    exi[3] = (1 << 2) | //write
        (1 << 1) | //use DMA
        (1 << 0); //start now
    while(exi[3] & 1); //wait for transfer
    exi[0] = prev0;
}

bool bPause = true;

int main(int argc, char **argv) {
    int err = init();
    if(err) quit("Startup failed\n");

    IRQ_Request(IRQ_EXI1_EXI, isrExi1Exi, NULL);
    IRQ_Request(IRQ_EXI1_EXT, isrExi1Ext, NULL);
    IRQ_Request(IRQ_EXI1_TC,  isrExi1Tc,  NULL);
    __UnmaskIrq(IM_EXI1);
    _exiReg[(1*5)+0] |= (1 << 0) | //enable EXI interrupt
        (1 << 10) | //enable EXT interrupt
        (1 << 2); //enable TC interrupt

    int selCh = 0;
    //u32 data  = 0xB410B420;
    u32 data  = 0xAABBCCDD;
    while(1) {
        VIDEO_WaitVSync();
        PAD_ScanPads();
        printf(
            "\x1B[2;0H\x1B[2K" //home, clear line
            "%s.\n", bPause ? "PAUSED  " : "Running");

        if(!bPause) {
            for(int iChan=0; iChan<3; iChan++) {
                int offs = iChan * 5;
                u32 stat = _exiReg[offs];
                _exiReg[offs+3] =
                    (3 << 4) | //4 bytes
                    (0 << 2) | //read
                    (0 << 1) | //immediate
                    (1 << 0); //start
                while(_exiReg[offs+3] & 1); //wait for transfer
                printf("%c%d %08X %c%c%c %c%c%c\n",
                    (iChan == selCh) ? '>' : ' ', iChan, stat,
                    (stat & (1 << 12)) ? 'C' : '.', //device connected
                    (stat & (1 << 11)) ? 'I' : '.', //external interrupt
                    (stat & (1 <<  1)) ? 'E' : '.', //external interrupt
                    (stat & (1 <<  9)) ? '2' : '-', //CS 2
                    (stat & (1 <<  8)) ? '1' : '-', //CS 1
                    (stat & (1 <<  7)) ? '0' : '-'  //CS 0
                );
                /*_exiReg[offs] |=
                    (1 << 11) | //clear interrupt
                    (1 <<  1);  //clear interrupt*/
            }
        }
        else {
            for(int iChan=0; iChan<3; iChan++) {
                printf("%c%d ........ ... ...\n",
                    (iChan == selCh) ? '>' : ' ', iChan);
            }
        }
        printf("[X] Speed: %2dMHz\n", 1 << speed);
        printf("[Y] CS:    %c%c%c\n",
            (cs & 0x04) ? '2' : '.',
            (cs & 0x02) ? '1' : '.',
            (cs & 0x01) ? '0' : '.');
        printf("[A] Write  [B] Pause  [Z] UART Test\n");

        printf("head=%4d tail=%4d EXI=%9d EXT=%9d TC=%9d\n",
            rxHead, rxTail, exi1cnt, ext1cnt, tc1cnt);
        for(int i=0; i<128; i++) {
            printf("%08X%s", rxBuf[i],
                ((i & 7) == 7) ? "\r\n" : " ");
        }

        u32 bDown = PAD_ButtonsDown(PAD_CHAN0);
        u32 bHeld = PAD_ButtonsHeld(PAD_CHAN0);
        if(bDown & PAD_BUTTON_START) quit(NULL);
        if(bHeld & PAD_TRIGGER_Z) {
            doUartTest(NULL);
            //doUartTest("boot game\n");
            //doUartTest("Locked cache machine check handler installed\n");
            //doUartTest("ISR[0000] = AABBCCDD 12345678\r\n");
        }
        if(bDown & PAD_BUTTON_X) {
            speed++;
            if(speed > 5) speed = 0;
            _exiReg[(selCh * 5)+0x00] =
                (cs << 7) |
                (1 << 13) | //ROMDIS
                //(1 <<  0) | //enable interrupt
                //(1 << 10) | //enable interrupt
                (speed << 4);
        }
        if(bDown & PAD_BUTTON_Y) {
            cs = (cs + 1) & 7;
            _exiReg[(selCh * 5)+0x00] =
                (cs << 7) |
                (1 << 13) | //ROMDIS
                //(1 <<  0) | //enable interrupt
                //(1 << 10) | //enable interrupt
                (speed << 4);
        }
        if(bDown & PAD_BUTTON_A) {
            _exiReg[(selCh*5)+4] = data;
            //data = ~data;
            _exiReg[(selCh*5)+3] =
                (3 << 4) | //4 bytes
                //(1 << 4) | //2 bytes
                (1 << 2) | //write
                (0 << 1) | //immediate
                (1 << 0); //start
            while(_exiReg[(selCh*5)+3] & 1); //wait for transfer
        }
        if(bDown & PAD_BUTTON_B) bPause = !bPause;
        if(bDown & PAD_BUTTON_UP) selCh--;
        if(bDown & PAD_BUTTON_DOWN) selCh++;
        if(bDown & PAD_TRIGGER_L) {
            static u8 count = 0;
            _ipcReg[63] |= 0x00FF0000;
            u32 gpio = _ipcReg[48];
            gpio &= ~0x00FF0000; //debug port
            count++;
            gpio |= (count << 16);
            gpio ^= 0x20; //disc LED
            _ipcReg[48] = gpio;
        }
        if(selCh > 2) selCh = 0;
        if(selCh < 0) selCh = 2;
    }
	return 0;
}

/* this seems to be working but we need to think about a protocol.
with SPI, every time a byte is shifted out, a byte is shifted in.
so you always transmit and receive at the same time, never just
one or the other.
typically one side is sending dummy bytes while receiving from
the other side.
so to send we should do something like:
- keep reading bytes until we get a magic number that indicates
  it's ready to receive (we're sending some nonsense during this)
- send the data (and receive nonsense)
I think this is similar to SD cards

for sending to the game we can assert one of the interrupt lines
so that the game will read
*/
