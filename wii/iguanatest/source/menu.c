#include "main.h"

int cursor = 0;

static const char *opcodes[] = {
    "NOP",      "SEND1", "SEND2",    "SEND3",
    "SENDBULK", "RECV",  "RECVBULK", "HWCTL",
    "QUERY",    "N/A",   "N/A",      "N/A",
    "N/A",      "N/A",   "N/A",      "N/A"};

void draw_speed(const char *fmt) {
    printf(fmt, 1 << speed);
    //officially, the highest two settings are invalid,
    //but this code will display them as 64MHz and 128MHz.
    //my particular Wii does seem to be doing something
    //different at "64MHz" but can't confirm if it's
    //really 64MHz. it does seem to be faster than 32MHz,
    //but the data is coming through a bit garbled,
    //probably because my device can't keep up...
    //mainly it's shifted left (device misses a cycle)
    //and extra ones sometimes appear at the end.
    //128MHz acts like something less than 32MHz,
    //guessing it's 1MHz?
}
void adj_speed(int amount) {
    if(!amount) amount = 1;
    speed += amount;
    if(speed < 0) speed = 7;
    if(speed > 7) speed = 0;
}

void draw_data(const char *fmt) {
    static const char *modes[] = {"Hello",
        "Text", "Payload", "All Zero", "All One",
        "0101...", "1010...", "Random", NULL};
    printf(fmt, modes[dataMode]);
}
void adj_data(int amount) {
    if(!amount) amount = 1;
    dataMode += amount;
    if(dataMode >= NUM_DATA_MODES) dataMode = 0;
    if(dataMode < 0) dataMode = NUM_DATA_MODES - 1;
}

void draw_test(const char *fmt) {
    static const char *modes[] = {
        "Read", "Write", "Read/Write", NULL};
    printf(fmt, modes[testMode]);
}
void adj_test(int amount) {
    if(!amount) amount = 1;
    testMode += amount;
    if(testMode >= NUM_TEST_MODES) testMode = 0;
    if(testMode < 0) testMode = NUM_TEST_MODES - 1;
}

void draw_bulk(const char *fmt) {
    printf(fmt, useBulk ? "On" : "Off");
}
void adj_bulk(int amount) {
    useBulk = !useBulk;
}

void draw_payload0(const char *fmt) { printf(fmt, payload[0], opcodes[payload[0] >> 4]); }
void draw_payload1(const char *fmt) { printf(fmt, payload[1]); }
void draw_payload2(const char *fmt) { printf(fmt, payload[2]); }
void draw_payload3(const char *fmt) { printf(fmt, payload[3]); }
void adj_payload0(int amount) { payload[0] += amount; }
void adj_payload1(int amount) { payload[1] += amount; }
void adj_payload2(int amount) { payload[2] += amount; }
void adj_payload3(int amount) { payload[3] += amount; }

void draw_debugPort(const char *fmt) {
    printf(fmt, debugPort);
}
void adj_debugPort(int amount) {
    if(!amount) amount = 1;
    debugPort += amount;
}

void draw_autoWrite(const char *fmt) {
    printf(fmt, autoWrite ? "On" : "Off");
}
void adj_autoWrite(int amount) {
    autoWrite = !autoWrite;
}

void draw_dma(const char *fmt) {
    printf(fmt, useDma ? "On" : "Off");
}
void adj_dma(int amount) {
    useDma = !useDma;
}

void draw_writeNow(const char *fmt) {
    printf(fmt);
}
void adj_writeNow(int amount) {
    if(!amount) doExec();
}

static struct {
    const char *fmt;
    void (*draw)(const char*);
    void (*adjust)(int);
} menuItems[] = {
    {"Speed:      %3dMHz", draw_speed, adj_speed},
    {"DMA:         %3s", draw_dma, adj_dma},
    {"Data:        %-13s", draw_data, adj_data},
    {"Mode:        %-10s", draw_test, adj_test},
    {"Bulk Mode:   %3s", draw_bulk, adj_bulk},
    {"Payload:     %02X...... %-8s", draw_payload0, adj_payload0},
    {"Payload:     ..%02X....", draw_payload1, adj_payload1},
    {"Payload:     ....%02X..", draw_payload2, adj_payload2},
    {"Payload:     ......%02X", draw_payload3, adj_payload3},
    {"Auto Exec:   %3s (or hold B)", draw_autoWrite, adj_autoWrite},
    {"Exec Now",   draw_writeNow, adj_writeNow},
    {"DebugPort:   0x%02X", draw_debugPort, adj_debugPort},
    {NULL, NULL, NULL} //end of list
};

int numItems = 0;
void drawMenu() {
    numItems = 0;
    for(int i=0; menuItems[i].fmt; i++) {
        printf((i == cursor) ? ">> " : "   ");
        menuItems[i].draw(menuItems[i].fmt);
        printf("\r\n");
        numItems++;
    }
}

void runMenu() {
    u32 bDown = PAD_ButtonsDown(PAD_CHAN0);
    u32 bHeld = PAD_ButtonsHeld(PAD_CHAN0);
    if(bDown & PAD_BUTTON_START) quit(NULL);

    void (*adjust)(int) = menuItems[cursor].adjust;
    if(bHeld & PAD_TRIGGER_Z) bDown = bHeld; //hold for constant adjustment
    if(bDown & PAD_BUTTON_A) adjust(0);
    else if(bDown & PAD_BUTTON_LEFT) adjust(-1);
    else if(bDown & PAD_BUTTON_RIGHT) adjust(1);
    else if(bDown & PAD_BUTTON_Y) adjust(-0x10);
    else if(bDown & PAD_BUTTON_X) adjust(0x10);
    else if(bDown & PAD_TRIGGER_L) adjust(-0x100);
    else if(bDown & PAD_TRIGGER_R) adjust(0x100);
    else if(bDown & PAD_BUTTON_UP) cursor -= 1;
    else if(bDown & PAD_BUTTON_DOWN) cursor += 1;
    if(cursor < 0) cursor = numItems - 1;
    if(cursor >= numItems) cursor = 0;
}
