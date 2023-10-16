#include "main.h"

#define CMDBUF_SIZE 1024
static char *cmdBuf = NULL;
static bool redraw, quit;

s8 chrToHex(char chr) {
    if     (chr >= '0' && chr <= '9') return  chr - '0';
    else if(chr >= 'A' && chr <= 'F') return (chr - 'A') + 10;
    else if(chr >= 'a' && chr <= 'f') return (chr - 'a') + 10;
    return -1;
}

u32 readHex(char *in, char **out) {
    u32 result = 0;
    while(*in == ' ') in++; //skip spaces
    while(*in == '0') in++; //skip leading zeros
    if(*in == 'x') in++; //skip "0x"
    while(*in) {
        s8 v = chrToHex(*in);
        if(v < 0) break;
        result = (result << 4) | (u32)(v & 0xFF);
        in++;
    }
    *out = in;
    return result;
}

void cmd_help(char *param) {
    for(int i=0; debugConsoleCmds[i].cmd; i++) {
        exiPrintf("%-10s %s\r\n",
            debugConsoleCmds[i].cmd, debugConsoleCmds[i].help);
    }
}

void cmd_quit(char *param) {
    quit = true;
}

void cmd_read(char *param) {
    u32 addr  = readHex(param, &param);
    u32 count = readHex(param, &param);
    if(!count) count = 4;
    //exiPrintf("Dump 0x%X from 0x%08X\r\n", count, addr);
    dumpMem(addr, count);
    exiPuts("\r\n");
}

void cmd_write(char *param) {
    u32 addr = readHex(param, &param);
    u8 *data = (u8*)addr;
    while(true) {
        while(*param == ' ') param++;
        s8 c1 = chrToHex(*(param++));
        if(c1 < 0) break;

        while(*param == ' ') param++;
        s8 c2 = chrToHex(*(param++));
        if(c2 < 0) break;

        *(data++) = (c1 << 4) | c2;
    }
    u32 len = (u32)data - addr;
    DCFlushRange((void*)addr, len);
    ICInvalidateRange((void*)addr, len);
}

void cmd_threads(char *param) {
    dumpThreads();
}

void cmd_dumpdvd(char *param) {
    dvdDumpPendingReadCallbacks();
    dvdDumpPendingCancelCallbacks();
    dvdDumpPendingStreamCallbacks();
    dvdDumpOpenFiles();
}

void cmd_dumpmem(char *param) {
    writeMemDump();
}

void cmd_dumpstack(char *param) {
    dumpStack();
}

void cmd_dumpregs(char *param) {
    u32 gpr[32];
    __asm__ __volatile__ (
        "stmw 0, 0(%0)"
        :: "r" (gpr));
    for(int i=0; i<32; i += 4) {
        exiPrintf("r%2d=%08X r%2d=%08X r%2d=%08X r%2d=%08X\r\n",
            i, gpr[i], i+1, gpr[i+1], i+2, gpr[i+2], i+3, gpr[i+3]);
    }
}

void cmd_call(char *param) {
    u32 addr = readHex(param, &param);
    u32 arg0 = readHex(param, &param);
    u32 arg1 = readHex(param, &param);
    u32 arg2 = readHex(param, &param);
    u32 arg3 = readHex(param, &param);
    u32 (*func)(u32,u32,u32,u32) = addr;
    u32 res = func(arg0, arg1, arg2, arg3);
    exiPrintf("Result: 0x%08X\r\n", res);
}

void cmd_hook(char *param) {
    u32 addr = readHex(param, &param);
    u32 target = readHex(param, &param);
    u32 isBl = readHex(param, &param);
    u32 forceTrampoline = readHex(param, &param);
    u32 oldVal = *(u32*)addr;
    hookBranch(addr, target, isBl, forceTrampoline);
    exiPrintf("Old value: 0x%08X\r\n", oldVal);
}

void cmd_break(char *param) {
    u32 addr = readHex(param, &param);
    addr |= 0xC0000000; //enable bkpt; enable xlate
    __asm__ __volatile__ (
        "mtspr 1010, %0"
        : "=r" (addr) :);
}

void cmd_reset(char *param) {
    _ipcReg[0x194>>2] = 0; //reset system
}

DebugConsoleCommand debugConsoleCmds[] = {
    {"?", "Show help", cmd_help},
    {"q", "Exit debugger", cmd_quit},
    {"r", "Read memory", cmd_read},
    {"w", "Write memory", cmd_write},
    {"c", "Call function", cmd_call},
    {"h", "Hook function", cmd_hook},
    {"regs", "Dump GPRs", cmd_dumpregs},
    {"threads", "Display thread states", cmd_threads},
    {"dumpdvd", "Display DVD state", cmd_dumpdvd},
    {"dumpmem", "Write memdump file", cmd_dumpmem},
    {"dumpstack", "Display stack", cmd_dumpstack},
    {"break", "Set instr breakpoint", cmd_break},
    {"reset", "Reboot system", cmd_reset},
    {NULL, NULL, NULL} //end
};

void execCmd() {
    if(!cmdBuf[0]) return;

    char *param = NULL;
    for(int i=0; i<CMDBUF_SIZE; i++) {
        if(cmdBuf[i] == ' ') {
            cmdBuf[i] = 0;
            param = &cmdBuf[i+1];
            break;
        }
        else if(cmdBuf[i] == 0) break;
    }
    for(int i=0; debugConsoleCmds[i].cmd; i++) {
        if(!strcmp(debugConsoleCmds[i].cmd, cmdBuf)) {
            debugConsoleCmds[i].func(param);
            return;
        }
    }
    exiPuts("Invalid command\r\n");
}

void handleChr(char chr) {
    //exiPrintf("chr=%02X\r\n", chr);
    int len = strlen(cmdBuf);
    switch(chr) {
        case '\b': case 0x7F:
            if(len) cmdBuf[len-1] = 0;
            break;

        case '\t': break;

        case '\r': case '\n':
            exiPuts("\r\n");
            execCmd();
            //fall thru
        case 3: //Ctrl+C
            cmdBuf[0] = 0;
            exiPuts("\r\n");
            break;

        default:
            if(len+1 < CMDBUF_SIZE) {
                cmdBuf[len] = chr;
                cmdBuf[len+1] = 0;
            }
    }
}

void interactiveDebugger(int excCode) {
    //if(!haveGecko) return;
    if(excCode) exiPrintf(" *** ERROR %d ***\r\n", excCode);
    else exiPuts(" *** DEBUGGER START ***\r\n");
    //exiPrintf("MSR: 0x%08X\r\n", mfmsr());

    char _buf[CMDBUF_SIZE];
    cmdBuf = _buf;
    memset(_buf, 0, sizeof(_buf));
    redraw = true;
    quit = false;

    while(!quit) {
        if(redraw) {
            exiPrintf("\r> %s\x1B[K", cmdBuf); //clear from cursor to EOL
            redraw = false;
        }
        u32 val = iguanaReadWrite(0x50000000); //RECV
        u32 respType = val >> 28;
        u32 body = (val >> 2) & 0xFFFFFF;
        switch(respType) {
            case 4: //RX1
                handleChr(body & 0xFF);
                redraw = true;
                break;

            case 5: //RX2
                handleChr((body >> 8) & 0xFF);
                handleChr(body & 0xFF);
                redraw = true;
                break;

            case 6: //RX3
                handleChr((body >> 16) & 0xFF);
                handleChr((body >> 8) & 0xFF);
                handleChr(body & 0xFF);
                redraw = true;
                break;
        }
    }

    exiPuts(" *** DEBUGGER EXIT ***\r\n");
    cmdBuf = NULL;
}
