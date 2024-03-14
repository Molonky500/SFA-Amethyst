#include "main.h"

void putHex(char *dst, u32 num) {
    //snprintf doesn't work in an exception handler
    //static const char *hex = "0123456789ABCDEF";
    static const char *hex = "0123456789abcdef";
    //lowercase is nice to paste into objdump | less
    for(int i=0; i<8; i++) {
        *(dst++) = hex[(num >> 28) & 0xF];
        num <<= 4;
    }
}

size_t fixCrlf(const char *bufIn, char *bufOut, size_t lenOut) {
    //replace "\n" with "\r\n"
    const char *startOut = bufOut;
    const char *endOut = &bufOut[lenOut];
    char p = 0;
    while(bufOut < endOut) {
        char c = *(bufIn++);
        if(c == '\n' && p != '\r') *(bufOut++) = '\r';
        *(bufOut++) = c;
        if(!c) break;
        p = c;
    }
    return bufOut - startOut;
}

//same hook is used for several debug print functions
//that are stubbed in the game binary.
//XXX any reason we can't do this on the GC side instead?
//only the lack of vsnprintf which we could port?
void osPrintHook(const char *fmt, ...) {
    char buf[1024], buf2[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    fixCrlf(buf, buf2, sizeof(buf2));
    exiPuts(buf2);
    va_end(args);
    //dumpStack();
}

void dumpMem(void *addr, uint32_t count) {
    uint32_t *dumpCode = (uint32_t*)addr;
    for(uint32_t i=0; i<count / 4; i++) {
        if((i&3)==0) exiPrintf("%08X: ", (uint32_t)dumpCode);
        exiPrintf("%08X%s", *(dumpCode++),
            ((i&3)==3) ? "\r\n" : " ");
    }
}

void dumpStack() {
    char msg[1024];
    strcpy(msg, "LR = ........ F = ........\r\n");
    putHex(&msg[ 5], __builtin_return_address(0));
    putHex(&msg[18], __builtin_frame_address(0)); //stack pointer
    exiPuts(msg);
    strcpy(msg, "-> ........ [........]\r\n");

    u32 *sp = (u32*)__builtin_frame_address(0);
    while((u32)sp >= 0x80000000 && (u32)sp <= 0x93FFFFFF) {
        putHex(&msg[ 3], sp[1]);
        putHex(&msg[13], sp);
        exiPuts(msg);
        sp = (u32*)*sp;
    }
    exiPuts("-- end\r\n");
}

const char* getThreadStateName(int state) {
    switch(state) {
        case OS_THREAD_STATE_READY:    return "Ready";
        case OS_THREAD_STATE_RUNNING:  return "Running";
        case OS_THREAD_STATE_WAITING:  return "Waiting";
        case OS_THREAD_STATE_MORIBUND: return "Moribund";
        default: return "?";
    }
}

void dumpThreads() {
    exiPuts("Thread dump:\r\n");
    OSThread *thread = *(OSThread**)0x800000dc;
    while(thread) {
        u32 stackTop = (u32)thread->stackBase; //high addr
        u32 stackBot = (u32)thread->stackEnd;  //low  addr
        u32 *sp      = (u32*)thread->context.gpr[1];
        u32 pc       = (u32)thread->context.srr0;

        const char *name = "unknown";
        PrevThreadState *pState = findThread(thread);
        if(pState) {
            if(pState->name != NULL) name = pState->name;
        }
        exiPrintf("Thread %08X (%s): PC=%08x stack=%08x-%08x\r\n", thread, name, pc,
            stackBot, stackTop);
        exiPrintf("State: %02X %s; detach: %s; err: 0x%08X; exit: 0x%08X; prio: %d (base %d)\r\n",
            thread->state, getThreadStateName(thread->state),
            (thread->attr & 1) ? "yes" : "no",
            thread->error, (u32)thread->val, thread->priority, thread->base);
        for(int i=0; i<32; i += 4) {
            for(int j=0; j<4; j++) {
                exiPrintf("  r%2d: %08x", (i+j), (u32)thread->context.gpr[i+j]);
            }
            exiPrintf("\r\n");
        }
        while(PTR_VALID(sp)) {
            exiPrintf(" > %08x\r\n", sp[1]);
            sp = (u32*)*sp;
        }
        //exiPrintf("next=%08x prev=%08x", thread->linkActive.next, thread->linkActive.prev);
        thread = thread->linkActive.next;
    }
}
