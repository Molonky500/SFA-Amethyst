#include "main.h"

static u32 *trampoline = (u32*)0x80000200;

uint32_t hookBranch(uint32_t addr, void *target, int isBl) {
    /** Replace a `b` or `bl` instruction in memory.
     *  @param addr Address to hook.
     *  @param target Function to branch to.
     *  @param isBl whether to insert a `b` or a `bl`.
     *  @return The original destination address.
     *  @note The target address should be a `b` or `bl` instruction.
     *  Otherwise, the return value is not useful, and special care needs to
     *  be taken to prevent the injected code from interfering with the
     *  original code's register state.
     */
    uint32_t oldOp = *(uint32_t*)addr;
    s32 dist = (s32)target - (s32)addr;
    if(abs(dist) >= 32*1024*1024) {
        //can't reach with a single branch
        uint32_t *code = (uint32_t*)addr;
        if((oldOp & ~0x03FFFFFF) == 0x48000000) {
            //replacing a b or bl
            if((u32)trampoline >= 0x80000300) {
                exiPrintf("Too many trampolines (%08X)\n", addr);
                while(1);
            }

            uint32_t relDest = (uint32_t)trampoline - addr;
            *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;

            exiPrintf("trampoline at %08X -> %08X op %08X -> %08X (bl=%d)\n",
                addr, trampoline, oldOp, *(uint32_t*)addr, isBl);
            *(trampoline++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(trampoline++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(trampoline++) = 0x7D8903A6; //mtspr CTR,r12
            *(trampoline++) = 0x4E800420; //bctr
            DCInvalidateRange((void*)&trampoline[-4], 32);
        }
        else {
            exiPrintf("direct long jump at %08X (bl=%d)\n",
                addr, isBl);
            *(code++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(code++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(code++) = 0x7D8903A6; //mtspr CTR,r12
            *(code++) = 0x4E800420 | (isBl ? 1 : 0); //bctr or bctrl
        }
        DCInvalidateRange((void*)addr, 32);
    }
    else {
        //make b or bl opcode
        uint32_t relDest = (uint32_t)target - addr;
        *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;
        exiPrintf("patch short jump at 0x%08X op 0x%08X (bl=%d)\n",
            addr, *(uint32_t*)addr, isBl);
        DCInvalidateRange((void*)addr, 32);
    }

    //decode original instruction and return original destination
    return addr + (oldOp & 0x03FFFFFC);
}

static void osPrintHook(const char *fmt, ...) {
    switchToOgc();
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);
    int len = strlen(buf);
    if(buf[len-1] != '\n') exiPuts("\n");
    va_end(args);
    //sprintf can't be used with interrupts disabled
    //exiPuts(fmt);
    switchToGame();
}

static void dspDebugPrint_hook(const char *fmt, ...) {
    switchToOgc();
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    exiPuts(buf);
    int len = strlen(buf);
    if(buf[len-1] != '\n') exiPuts("\n");
    va_end(args);
    switchToGame();
}

uint OSGetFontEncode_hook() {
    return 0; //English
    //return 1; //Japanese
}

char* OSGetFontWidth_hook(char* string, s32* width) {
    *width = 8;
    return &string[1];
}

char* OSGetFontTexel_hook(char* string, void* image, s32 pos,
s32 stride, s32* width) {
    return &string[1];
}

void doPatches(DolHeader *header) {
    //hookBranch(0x80000100, _raw_exceptionHook_Reset, 0);
    //hookBranch(0x80000200, _raw_exceptionHook_MachineCheck, 0);
    //hookBranch(0x80000300, _raw_exceptionHook_DSI, 0);
    //hookBranch(0x80000400, _raw_exceptionHook_ISI, 0);
    //hookBranch(0x80000500, _raw_exceptionHook_External, 0);
    //hookBranch(0x80000600, _raw_exceptionHook_Alignment, 0);
    //hookBranch(0x80000700, _raw_exceptionHook_Program, 0);
    //hookBranch(0x80000800, _raw_exceptionHook_FpUnavailable, 0);
    //hookBranch(0x80000900, _raw_exceptionHook_Decrementer, 0);
    //hookBranch(0x80000C00, _raw_exceptionHook_Syscall, 0);
    //hookBranch(0x80000D00, _raw_exceptionHook_Trace, 0);
    //hookBranch(0x80000F00, _raw_exceptionHook_PerfMon, 0);
    //hookBranch(0x80001300, _raw_exceptionHook_IABR, 0);
    //hookBranch(0x80001700, _raw_exceptionHook_Thermal, 0);
    hookBranch(0x8007d6dc, osPrintHook, 0);
    hookBranch(0x802510cc, dspDebugPrint_hook, 0);
    hookBranch(0x80246a60, OSSleepThread_hook, 0);
    //hookBranch(0x802462a8, OSCreateThread_hook, 0);
    hookBranch(0x801175e4, OSCreateThread_hook, 1);
    hookBranch(0x80117614, OSCreateThread_hook, 1);
    hookBranch(0x801196c0, OSCreateThread_hook, 1);
    hookBranch(0x80119b98, OSCreateThread_hook, 1);
    hookBranch(0x80119bc8, OSCreateThread_hook, 1);
    hookBranch(0x80137de0, OSCreateThread_hook, 1);
    hookBranch(0x802464ac, OSCancelThread_hook, 0);
    hookBranch(0x80246668, OSResumeThread_hook, 0);
    hookBranch(0x802468f0, OSSuspendThread_hook, 0);
    hookBranch(0x80246b4c, OSWakeupThread_hook, 0);
    hookBranch(0x80245d88, OSGetCurrentThread_hook, 0);
    //hookBranch(0x80242474, OSClearContext_hook, 0);
    hookBranch(0x80246078, OSSwitchThreadOrSomeShit_hook, 0);
    //hookBranch(0x80242394, OSLoadContext_hook, 0);
    //hookBranch(0x80242314, OSSaveContext_hook, 0);
    //hookBranch(0x802422ac, OSSetCurrentContext_hook, 0);

    hookBranch(0x80245d78, OSInitThreadQueue_hook, 0);
    hookBranch(0x80244000, OSInitMessageQueue_hook, 0);
    hookBranch(0x80244060, OSSendMessage_hook, 0);
    hookBranch(0x80244128, OSReceiveMessage_hook, 0);

    //hookBranch(0x8024377c, OSDisableInterrupts_hook, 0);
    //hookBranch(0x80243790, OSEnableInterrupts_hook, 0);
    //hookBranch(0x802437a4, OSRestoreInterrupts_hook, 0);
    hookBranch(0x80243b44, OSMaskInterrupts_hook, 0);
    hookBranch(0x80243bcc, OSUnmaskInterrupts_hook, 0);

    hookBranch(0x80248b9c, DVDOpen_hook, 0);
    hookBranch(0x80015850, DVDRead_hook, 0);
    hookBranch(0x80248f9c, DVDReadPrio_hook, 0);
    hookBranch(0x80248c64, DVDClose_hook, 0);
    hookBranch(0x80248eac, DVDReadAsyncPrio_hook, 0);
    hookBranch(0x802490d8, DVDPrepareStreamAsync_hook, 0);
    hookBranch(0x8024afd8, DVDCancelStreamAsync_hook, 0);

    hookBranch(0x80242f20, OSGetFontEncode_hook, 0);
    hookBranch(0x8024363c, OSGetFontWidth_hook, 0);
    hookBranch(0x80243338, OSGetFontTexel_hook, 0);

    hookBranch(0x8024ffe4, ARStartDMA_hook, 0);
    hookBranch(0x8024f6fc, AIInitDMA_hook, 0);

    //this doesn't work nicely because AR DMA apparently
    //doesn't like being done from 92xxxxxx
    //and once we've used MEM2 it never goes back to MEM1
    //hookBranch(0x80023cc8, alloc_hook, 0);
    //hookBranch(0x800233e8, free_hook, 0);

    static const u32 patches[] = {
        //address, value

        //set some header values
        0x80000000, 0x47534145,
        0x80000004, 0x30310000,
        0x80000008, 0x01000000,
        0x800000f0, 0x01800000,

        //disable heap shit
        //0x80020e58, 0x60000000,

        //don't call __check_pad3 since it's not there anymore
        0x80003260, 0x60000000,

        //use the pad3 value to instead store a magic value to
        //tell Amethyst hooks that we're in Wii mode.
        0x800030e4, 0xACAB7511,

        //don't overwrite syscall handler
        //0x802406b4, 0x60000000,

        //don't mask off all the interrupts
        //0x80240354, 0x60000000,
        //0x802406c0, 0x60000000,

        //don't do syscalls
        //0x80241a48, 0x60000000,
        //0x8024037c, 0x60000000,

        //don't replace our external interrupt handler
        0x80243854, 0x60000000,
        0x802406b0, 0x60000000,

        //disable padUpdate
        //0x80014f40, 0x4E800020,

        //THP shit
        //0x80116224, 0x4E800020, //attractMode_init (THP)
        //0x803dd610, 0x00000004, //THP finished
        //0x80116260, 0x38600000, //skip THP load
        //0x80118018, 0x4E800020, //attractModeDoAudioDma
        //0x8024f784, 0x4E800020, //AIStartDMA

        //disable some thread stuff
        0x80137d28, 0x4E800020, //installBsodHandlers
        0x802406b0, 0x60000000, //OSExceptionInit
        //0x80021074, 0x60000000, //_initCardAndDsp
        //0x80240d34, 0x4E800020, //OSInitAlarm
        0x80240bc4, 0x4E800020, //__OSSetExceptionHandler
        0x802406b4, 0x60000000, //__OSInitSystemCall
        //0x802406d4, 0x60000000, //__OSContextInit
        //0x802406dc, 0x60000000, //exiInit
        //0x802406e8, 0x60000000, //_osInitThreadQueues
        //0x80243bcc, 0x4E800020, //__OSUnmaskInterrupts
        //0x80243bcc, 0x4E800020, //__OSMaskInterrupts
        //0x802437c8, 0x4E800020, //__OSSetInterruptHandler
        //0x80242474, 0x4E800020, //OSClearContext

        //set arena
        //0x803dc538, 0x803fa480,
        //0x803dde20, 0x817ea240,

        //set TV params
        //0x803dccf0, 0x8032e65c, //progressive scan mode

        //manually set some exception handlers
        0x803dddec, 0x80003000, //the exception handler table
        0x803dde38, 0x80003040, //the IRQ handler table
        //0x8000301c, 0x802427fc, //fpuUnavailableHandler
        //0x80003020, 0x80241390, //DecrementerExceptionHandler
        //0x80003054, 0x8024fcd4, //DSP IRQ
        //0x80003058, 0x802501a0, //ARInterruptHandler
        //0x8000305C, 0x8025111c, //DSPInterruptHandler
        //0x80003060, 0x8024fc58, //DSPInterruptHandler
        //0x80003084, 0x80255dbc, //GXCPInterruptHandler

        //tell game the OS is already initialized
        //0x803ddde8, 0x00000001,

        //disable some DVD stuff
        0x802491f4, 0x4E800020, //DVDInit
        0x802408fc, 0x60000000, //DVDInquiryAsync
        0x80015624, 0x4E800020, //dvdCheckError

        //disable audio for now
        //0x802406ec, 0x60000000, //__OSInitAudioSystem
        //0x80020ec8, 0x38600001, //audioInit
        //0x80009bf8, 0x4E800020, //other audioInit
        //0x8000d200, 0x4E800020, //streamPlay
        //0x8024afd8, 0x4E800020, //DVDCancelStreamAsync
        0x8024b094, 0x4E800020, //DVDStopStreamAtEndAsync
        //0x8024f6fc, 0x4E800020, //AIInitDMA
        //0x80284670, 0x4E800020, //audio
        //0x8024ffe4, 0x4E800020, //ARStartDMA
        //0x80284224, 0x4E800020, //waits for AR DMA
        //0x80284048, 0x4E800020, //waits for AR DMA
        //0x802843f0, 0x60000000, //don't wait for AR DMA

        //these do nothing lmao
        //0x80020c68, 0x60000000, //mainLoopAudioUpdate
        //0x80020c6c, 0x60000000, //mainLoopDoQueuedSounds

        //__ARCheckSize probes ARAM to detect size, but this
        //clobbers MEM2, so disable it and set the size
        //manually instead.
        0x80250218, 0x4E800020, //__ARCheckSize
        0x803de01c, 0x01000000, //__ARSize

        //0x8011937c, 0x38600001,
        //0x80102aa0, 0x4E800020, //Camera::triggerAction

        //bypass some init by telling the game
        //it's doing a soft reset
        //0x803dddf8, 0x00000001,

        //0x8001a3f8, 0x60000000, //gameTextLoadSystemFonts
        0x8001a9ec, 0x60000000, //OSLoadFont
        //0x800154ac, 0x4E800020, //initControllers

        //titleTryLoadSaveFiles
        0x8007dbc0, 0x38600000,
        0x8007dbc4, 0x4E800020,

        //loadSaveGame
        0x8007dc5c, 0x38600000,
        0x8007dc60, 0x4E800020,

        //titleCreateCardFile
        0x8007dd04, 0x38600000,
        0x8007dd08, 0x4E800020,

        //_saveGame
        0x8007db24, 0x38600000,
        0x8007db28, 0x4E800020,

        0 //end of list
    };
    for(int i=0; patches[i]; i += 2) {
        (*(u32*)patches[i]) = patches[i+1];
        DCInvalidateRange((void*)patches[i], 32);
        ICInvalidateRange((void*)patches[i], 32);
    }
}
