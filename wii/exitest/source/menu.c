#include "main.h"

int cursor = 0;

void draw_channel(const char *fmt) {
    printf(fmt, chan+'A');
}
void adj_channel(int amount) {
    if(!amount) amount = 1;
    chan += amount;
    if(chan < 0) chan = 2;
    if(chan > 2) chan = 0;
}

void draw_device(const char *fmt) {
    printf(fmt, cs);
}
void adj_device(int amount) {
    if(!amount) amount = 1;
    cs += amount;
    if(cs < 0) cs = 2;
    if(cs > 2) cs = 0;
}

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

void draw_invert(const char *fmt) {
    printf(fmt, invert ? "Yes" : "No");
}
void adj_invert(int amount) {
    invert = !invert;
}

void draw_data(const char *fmt) {
    static const char *modes[] = {
        "Text", "Payload 8bit", "Payload 16bit", "Payload 32bit",
        "All Zero", "All One", "0101...", "1010...",
        "Echo", "Random", "Counter", NULL};
    printf(fmt, modes[dataMode]);
}
void adj_data(int amount) {
    if(!amount) amount = 1;
    dataMode += amount;
    if(dataMode >= NUM_DATA_MODES) dataMode = 0;
    if(dataMode < 0) dataMode = NUM_DATA_MODES - 1;
}

void draw_payload0(const char *fmt) { printf(fmt, payload[0]); }
void draw_payload1(const char *fmt) { printf(fmt, payload[1]); }
void draw_payload2(const char *fmt) { printf(fmt, payload[2]); }
void draw_payload3(const char *fmt) { printf(fmt, payload[3]); }
void adj_payload0(int amount) { payload[0] += amount; }
void adj_payload1(int amount) { payload[1] += amount; }
void adj_payload2(int amount) { payload[2] += amount; }
void adj_payload3(int amount) { payload[3] += amount; }

/*void draw_delay(const char *fmt) {
    printf(fmt, delay);
}
void adj_delay(int amount) {
    if(!amount) amount = 1;
    delay += amount;
    if(delay < 0) delay = 0;
}

void draw_nStart(const char *fmt) {
    printf(fmt, nStart);
}
void adj_nStart(int amount) {
    if(!amount) amount = 1;
    nStart += amount;
    if(nStart < 0) nStart = 0;
}

void draw_nStop(const char *fmt) {
    printf(fmt, nStop);
}
void adj_nStop(int amount) {
    if(!amount) amount = 1;
    nStop += amount;
    if(nStop < 0) nStop = 0;
}

void draw_reverse(const char *fmt) {
    printf(fmt, reverse ? "Yes" : "No");
}
void adj_reverse(int amount) {
    reverse = !reverse;
}*/

void draw_gecko(const char *fmt) {
    printf(fmt, useGecko ? "On" : "Off");
}
void adj_gecko(int amount) {
    useGecko = !useGecko;
}

void draw_proto(const char *fmt) {
    printf(fmt, isCustomProtocol ? "On" : "Off");
}
void adj_proto(int amount) {
    isCustomProtocol = !isCustomProtocol;
}

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

void draw_writeNow(const char *fmt) {
    printf(fmt);
}
void adj_writeNow(int amount) {
    if(!amount) doWrite();
}

static struct {
    const char *fmt;
    void (*draw)(const char*);
    void (*adjust)(int);
} menuItems[] = {
    {"Channel:     %c", draw_channel, adj_channel},
    {"Device (CS): %d", draw_device, adj_device},
    {"Speed:      %3dMHz", draw_speed, adj_speed},
    {"Data:        %13s", draw_data, adj_data},
    {"Payload:     %02X......", draw_payload0, adj_payload0},
    {"Payload:     ..%02X....", draw_payload1, adj_payload1},
    {"Payload:     ....%02X..", draw_payload2, adj_payload2},
    {"Payload:     ......%02X", draw_payload3, adj_payload3},
    //{"Delay:       %6duS", draw_delay, adj_delay},
    //{"StartBits:   %6d", draw_nStart, adj_nStart},
    //{"Stop Bits:   %6d", draw_nStop,  adj_nStop},
    //{"Invert:      %3s", draw_invert, adj_invert},
    //{"Reverse:     %3s", draw_reverse, adj_reverse},
    {"DebugPort:   0x%02X", draw_debugPort, adj_debugPort},
    {"Gecko Mode:  %3s (normally Channel B CS 1)      ", draw_gecko, adj_gecko},
    {"CustomProto: %3s", draw_proto, adj_proto},
    {"AutoWrite:   %3s (or hold B)", draw_autoWrite, adj_autoWrite},
    {"Write Now",       draw_writeNow, adj_writeNow},
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
