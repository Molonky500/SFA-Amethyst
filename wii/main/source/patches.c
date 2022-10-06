#include "main.h"

static u32 *trampoline = (u32*)0x80000200;

uint32_t hookBranch(uint32_t addr, void *target, bool isBl, bool forceTrampoline) {
    /** Replace a `b` or `bl` instruction in memory.
     *  @param addr Address to hook.
     *  @param target Function to branch to.
     *  @param isBl whether to insert a `b` or a `bl`.
     *  @param forceTrampoline whether to force the use of a trampoline.
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
        if(forceTrampoline
        || (oldOp & ~0x03FFFFFF) == 0x48000000) {
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

void doPatches() {
    //remap some HW regs
    u32 regRemap[] = {
        //EXI
        0x802439ac, 0x802439e8, 0x80243a4c, 0x80243d84,
        0x80243dbc, 0x80243df8, 0x80253458, 0x80253490,
        0x80253614, 0x80253670, 0x802536e0, 0x8025381c,
        0x802538a0, 0x802539a4, 0x80253e68, 0x80253f54,
        0x80254040, 0x80254130, 0x802543e4, 0x80285560,
        0x8028556c, 0x802855a0, 0x802856d4, 0x80285780,
        0x8028585c, 0x80285938, 0x80285a0c, 0x80285a90,
        0x80285c5c,
        //AI
        0x8024397c, 0x80243d60, 0x8024f838, 0x8024f878,
        0x8024f8a8, 0x8024f8d8, 0x8024f998, 0x8024f9cc,
        0x8024fa80, 0x8024fa90, 0x8024fabc, 0x8024fb8c,
        0x8024fc68, 0x8024fde4,
        0 //end of list
    };
    for(int i=0; regRemap[i]; i++) {
        u32 op = *(u32*)regRemap[i];
        if((op & 0xFFFF) != 0xCC00) {
            exiPrintf(" *** ERROR *** Incorrect entry %08X in regRemap\n",
                regRemap[i]);
        }
        else *(u32*)regRemap[i] = (op & 0xFFFF0000) | 0xCD00;
        DCInvalidateRange((void*)regRemap[i], 32);
        ICInvalidateRange((void*)regRemap[i], 32);
    }

    hookBranch(0x80014f40, padUpdate_hook, 0, 1);
    hookBranch(0x8007d6dc, osPrintHook, 0, 0);
    hookBranch(0x80246e04, osPrintHook, 0, 0);
    hookBranch(0x802510cc, osPrintHook, 0, 0);
    hookBranch(0x8024091c, OSExceptionInit_hook, 0, 0);
    hookBranch(0x80242a10, gameExceptionHook, 0, 0);
    hookBranch(0x802406c0, __OSInterruptInit_hook, 1, 0);
    hookBranch(0x80243b44, __OSMaskInterrupts_hook, 0, 0);
    hookBranch(0x80243bcc, __OSUnmaskInterrupts_hook, 0, 0);
    hookBranch(0x80243fe4, gameExtIrqHandler_hook, 0, 0);
    hookBranch(0x80248870, __DVDFSInit_hook, 0, 0);
    hookBranch(0x80248b9c, DVDOpen_hook, 0, 0);
    hookBranch(0x80015850, DVDRead_hook, 0, 0);
    hookBranch(0x80248f9c, DVDReadPrio_hook, 0, 0);
    //hookBranch(0x80248c64, DVDClose_hook, 0, 0);
    hookBranch(0x8024b428, DVDCancelAsync_hook, 0, 0);
    hookBranch(0x80248eac, DVDReadAsyncPrio_hook, 0, 0);
    hookBranch(0x802490d8, DVDPrepareStreamAsync_hook, 0, 0);
    hookBranch(0x8024afd8, DVDCancelStreamAsync_hook, 0, 0);
    hookBranch(0x8024ffe4, ARStartDMA_hook, 0, 0);
    hookBranch(0x8024f6fc, AIInitDMA_hook, 0, 0);
    hookBranch(0x8024f784, AIStartDMA_hook, 0, 0);
    //bypass the part of __ARCheckSize that clobbers MEM2
    //hookBranch(0x80250788, 0x80250b18, 0, 0);

    static const u32 patches[] = {
        //address, value

        //set some header values that apploader should set
        0x80000000, 0x47534145,
        0x80000004, 0x30310000,
        0x80000008, 0x01000000,
        0x800000f0, 0x01800000,

        0x80245980, 0x4E800020, //OSSetWirelessID

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
        //0x80243854, 0x60000000, //set ext irq handler
        //0x802406b0, 0x60000000, //OSExceptionInit

        //disable padUpdate
        //0x80014f40, 0x4E800020,

        //THP shit
        //0x80116224, 0x4E800020, //attractMode_init (THP)
        //0x803dd610, 0x00000004, //THP finished
        //0x80116260, 0x38600000, //skip THP load
        //0x80118018, 0x4E800020, //attractModeDoAudioDma
        //0x8024f784, 0x4E800020, //AIStartDMA

        //memcpyToCache: don't use LC
        //0x80022960, 0x380000FF,
        //0x80022a04, 0x380000FF,
        //0x80241c68, 0x4E800020,
        //0x80241c8c, 0x4E800020,
        //0x80241cb0, 0x4E800020,
        //0x80241d5c, 0x4E800020,
        //0x80241c08, 0x4E800020, //kill LCEnable

        //disable some thread stuff
        //0x80137d28, 0x4E800020, //installBsodHandlers
        //0x802406b0, 0x60000000, //OSExceptionInit
        //0x80021074, 0x60000000, //_initCardAndDsp
        //0x80240d34, 0x4E800020, //OSInitAlarm
        //0x80240bc4, 0x4E800020, //__OSSetExceptionHandler
        //0x802406b4, 0x60000000, //__OSInitSystemCall
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
        //0x803dddec, 0x80003000, //the exception handler table
        //0x803dde38, 0x80003040, //the IRQ handler table
        //those two don't work because the init code clears them.

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
        //0x802491f4, 0x4E800020, //DVDInit
        0x802408fc, 0x60000000, //DVDInquiryAsync
        0x80015624, 0x4E800020, //dvdCheckError
        //0x802492a4, 0x60000000, //__fstLoad

        //disable audio for now
        //0x802406ec, 0x60000000, //__OSInitAudioSystem
        //0x80020ec8, 0x38600001, //audioInit
        //0x80009bf8, 0x4E800020, //other audioInit
        //0x8000d200, 0x4E800020, //streamPlay
        //0x8024afd8, 0x4E800020, //DVDCancelStreamAsync
        //0x8024b094, 0x4E800020, //DVDStopStreamAtEndAsync
        //0x8024f6fc, 0x4E800020, //AIInitDMA
        //0x80284670, 0x4E800020, //audio
        //0x8024ffe4, 0x4E800020, //ARStartDMA
        //0x80284224, 0x4E800020, //waits for AR DMA
        //0x80284048, 0x4E800020, //waits for AR DMA
        //0x802843f0, 0x60000000, //don't wait for AR DMA
        //0x8024fcd4, 0x4E800020, //irq_DSP
        //0x802501a0, 0x4E800020, //__ARHandler
        //0x8025111c, 0x4E800020, //__DSPHandler
        //0x8024fc58, 0x4E800020, //__AISHandler
        //0x8024f9ac, 0x4E800020, //__AI_set_stream_sample_rate
        //0x8024f7d0, 0x4E800020, //AISetStreamPlayState
        //0x8024fabc, 0x4E800020, //AISetStreamVolLeft
        //0x8024fa90, 0x4E800020, //AISetStreamVolRight

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
        //0x8001a9ec, 0x60000000, //OSLoadFont
        //0x800154ac, 0x4E800020, //initControllers

        //titleTryLoadSaveFiles
        //0x8007dbc0, 0x38600000,
        //0x8007dbc4, 0x4E800020,

        //loadSaveGame
        //0x8007dc5c, 0x38600000,
        //0x8007dc60, 0x4E800020,

        //titleCreateCardFile
        //0x8007dd04, 0x38600000,
        //0x8007dd08, 0x4E800020,

        //_saveGame
        //0x8007db24, 0x38600000,
        //0x8007db28, 0x4E800020,

        0 //end of list
    };
    for(int i=0; patches[i]; i += 2) {
        (*(u32*)patches[i]) = patches[i+1];
        DCInvalidateRange((void*)patches[i], 32);
        ICInvalidateRange((void*)patches[i], 32);
    }
}
