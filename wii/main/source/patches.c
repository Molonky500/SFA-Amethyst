#include "main.h"

u32 gFrameCount = 0;
static u32 *trampoline = (u32*)0x800002a0;

void panic() {
    exiPuts("*** PANIC ***\r\n");
    interactiveDebugger(99);
    while(1) {
        SET_SCREEN_SOLID_YUV(76, 84, 255); //red
        udelay(250000);
        SET_SCREEN_SOLID_YUV(141, 191, 26); //light blue
        udelay(250000);
    }
}

u32 nextTrampoline(u32 cur) {
    if((cur+16) >= 0x80000C00) {
        exiPrintf(" *** ERROR *** Too many trampolines (reached %08X)\r\n", cur);
        panic();
    }
    //Axx, Bxx are unused
    if(cur < 0x80000900 && (cur+16) >= 0x80000900) return 0x800009a0;
    if(cur < 0x80000800 && (cur+16) >= 0x80000800) return 0x800008a0;
    if(cur < 0x80000700 && (cur+16) >= 0x80000700) return 0x800007a0;
    if(cur < 0x80000600 && (cur+16) >= 0x80000600) return 0x800006a0;
    if(cur < 0x80000500 && (cur+16) >= 0x80000500) return 0x800005a0;
    if(cur < 0x80000400 && (cur+16) >= 0x80000400) return 0x800004a0;
    if(cur < 0x80000300 && (cur+16) >= 0x80000300) return 0x800003a0;
    return cur;
}

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
            trampoline = nextTrampoline(trampoline);

            uint32_t relDest = (uint32_t)trampoline - addr;
            *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;

            //exiPrintf("trampoline at %08X -> %08X op %08X -> %08X (bl=%d)\r\n",
            //    addr, trampoline, oldOp, *(uint32_t*)addr, isBl);
            *(trampoline++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(trampoline++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(trampoline++) = 0x7D8903A6; //mtspr CTR,r12
            *(trampoline++) = 0x4E800420; //bctr
            DCStoreRange((void*)&trampoline[-4], 32);
            ICInvalidateRange((void*)&trampoline[-4], 32);
        }
        else {
            //exiPrintf("direct long jump at %08X (bl=%d)\r\n",
            //    addr, isBl);
            if(*code == 0x4E800420) {
                exiPrintf(" *** ERROR *** can't insert long jump at 0x%08X\r\n", addr);
                panic();
            }
            *(code++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(code++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(code++) = 0x7D8903A6; //mtspr CTR,r12
            *(code++) = 0x4E800420 | (isBl ? 1 : 0); //bctr or bctrl
        }
        DCStoreRange((void*)addr, 32);
        ICInvalidateRange((void*)addr, 32);
    }
    else {
        //make b or bl opcode
        uint32_t relDest = (uint32_t)target - addr;
        *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;
        //exiPrintf("patch short jump at 0x%08X op 0x%08X (bl=%d)\r\n",
        //    addr, *(uint32_t*)addr, isBl);
        DCStoreRange((void*)addr, 32);
        ICInvalidateRange((void*)addr, 32);
    }

    //decode original instruction and return original destination
    return addr + (oldOp & 0x03FFFFFC);
}

void mainLoopHook() {
    gFrameCount++;
    __UnmaskIrq(IM_EXI1); //we need this for debug

    //extern u8 __ipcbufferLo[], __ipcbufferHi[];
    //exiPrintf("IPC BUF %08X %08X  Arena1 %08X %08X  Arena2  %08X %08X\n",
    //    __ipcbufferLo, __ipcbufferHi,
    //    SYS_GetArena1Lo(), SYS_GetArena1Hi(),
    //    _mem2_heap_start, _mem2_heap_end);

    /*static bool triedInit = false;
    //curMap == warlock
    if((!triedInit) && (*(u32*)0x803dcec8) == 0xB) {
        triedInit = true;
        exiPuts("Telling thread to init Wiimote\r\n");
        bInitWiimote = 1;
    }*/
}

void cardUnlock_hook(void *addr, u32 size) {
    //change a param for Wii HW
    *(u32*)(addr+0xC) = 0x10000000;
    DCFlushRange(addr, size); //replaced
}

void LCEnable_hook() {
    exiPuts("REACHED LCEnable_hook\r\n");
    static void (*origFunc)(void) = 0x80241c08;
    origFunc();
    exiPuts("About to init game hooks\r\n");
    initGameHooks();
    exiPuts("Init game hooks OK\r\n");
}

int OSWakeupThread_hook(void *queue) {
    static int (*origFunc)(void) = 0x8024377c; //OSDisableInterrupts
    if(!queue) PANIC("OSWakeupThread(NULL)");
    return origFunc();
}

void _init_hook(u32 x,u32 y) { //for testing; not used
    //exiPrintf("Reached memInit; debuggerHookCode=0x%X pi2DebugFlag=0x%X debug=0x%X\r\n",
    //    *(u32*)0x80000060, *(u32*)0x803ddddc, *(u32*)0x803dde98);
    iguanaSetGreenLed(true);
    iguanaPutsNoFlush(" *** Reached hook ***\r\n");
    interactiveDebugger(0);
    void (*origFunc)(u32,u32) = 0x8024b418;
    origFunc(x,y);
    iguanaSetGreenLed(false);
    iguanaPutsNoFlush("Survived hook\r\n");
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
        //SI
        0x80251a00, 0x80251b30, 0x80251cc4, 0x80251cfc,
        0x80251e14, 0x80252064, 0x802522d4, 0x80252384,
        0x80252464, 0x80252518, 0x80252560, 0x802525c4,
        0x802525d4, 0x80252614, 0x802526b8, 0x8025272c,
        0x80252780, 0x802527d0, 0x80252868, 0x8025310c,
        //DI
        0x80240d20, 0x80247ae0, 0x80247b34, 0x80247bec,
        0x80247c28, 0x80247e90, 0x80247fd8, 0x80248288,
        0x80248308, 0x80248358, 0x802483ec, 0x80248478,
        0x8024850c, 0x802485a0, 0x80248638, 0x802486d8,
        0x80248748, 0x80248814, 0x80249260, 0x80249660,
        0x8024994c, 0x8024a0d8, 0x8024a440, 0x8024a46c,
        0x8024a4c8, 0x8024a50c, 0x8024a570, 0x8024a59c,
        0x8024a5d0, 0x8024a5f4, 0x8024a618, 0x8024a63c,
        0x8024a660, 0x8024a688, 0x8024a840, 0x8024aa0c,
        0x8024aa1c, 0x8024aa78, 0x8024b2ec, 0x8024b810,
        0 //end of list
    };
    for(int i=0; regRemap[i]; i++) {
        u32 op = *(u32*)regRemap[i];
        if((op & 0xFFFF) == 0xCD00) {
            //do nothing
        }
        else if((op & 0xFFFF) != 0xCC00) {
            exiPrintf(" *** ERROR *** Incorrect entry %08X in regRemap (%08X)\r\n",
                regRemap[i], op);
        }
        else *(u32*)regRemap[i] = (op & 0xFFFF0000) | 0xCD00;
        DCStoreRange((void*)regRemap[i], 32);
        ICInvalidateRange((void*)regRemap[i], 32);
    }

    hookBranch(0x8025fe1c, cardUnlock_hook, 1, 0);
    hookBranch(0x80246b64, OSWakeupThread_hook, 1, 0);

    hookBranch(0x80020c60, mainLoopHook, 1, 0); //patch unused main loop func
    hookBranch(0x8007d6dc, osPrintHook, 0, 0);
    hookBranch(0x80246e04, osPrintHook, 0, 0);
    hookBranch(0x802510cc, osPrintHook, 0, 0);
    hookBranch(0x8024091c, OSExceptionInit_hook, 0, 0);
    hookBranch(0x80242bf8, gameExceptionHook, 1, 0);
    hookBranch(0x80137df8, gameBsodHook, 0, 0);
    hookBranch(0x802406c0, __OSInterruptInit_hook, 1, 0);
    hookBranch(0x80243b44, __OSMaskInterrupts_hook, 0, 0);
    hookBranch(0x80243bcc, __OSUnmaskInterrupts_hook, 0, 0);
    hookBranch(0x80243fe4, gameExtIrqHandler_hook, 0, 0);
    hookBranch(0x802408ac, OSEnableInterrupts_hook, 1, 0);
    hookBranch(0x80020dbc, LCEnable_hook, 1, 0);
    hookBranch(0x80248b9c, DVDOpen_hook, 0, 0);
    hookBranch(0x80015850, DVDRead_hook, 0, 0);
    hookBranch(0x80248f9c, DVDReadPrio_hook, 0, 0);
    hookBranch(0x8024b428, DVDCancelAsync_hook, 0, 0);
    hookBranch(0x80248eac, DVDReadAsyncPrio_hook, 0, 0);
    hookBranch(0x802490d8, DVDPrepareStreamAsync_hook, 0, 0);
    hookBranch(0x8024afd8, DVDCancelStreamAsync_hook, 0, 0);
    hookBranch(0x8024b094, DVDStopStreamAtEndAsync_hook, 0, 0);
    hookBranch(0x8024f7d0, AISetStreamPlayState_hook, 0, 0);
    hookBranch(0x80244a58, OSRebootHook, 1, 0);
    hookBranch(0x8024ffe4, ARStartDMA_Hook, 0, 0);
    hookBranch(0x8000d528, playStream_hook, 1, 0);
    hookBranch(0x80009be4, mainLoopUpdateStream_hook, 1, 0);
    hookBranch(0x80020604, dvdMainLoopHook, 0, 1);
    hookBranch(0x8025400c, exiInterrupt_hook, 0, 0);
    hookBranch(0x802456c4, OSGetSoundMode_hook, 0, 0);

    static const u32 patches[] = {
        //address, value

        //temp stuff to try to fix Wii mode
        //0x80021074, 0x60000000, //do not init DSP
        //0x802848d8, 0x4E800020, //do not upload ucode
        //0x80281044, 0x4E800020, //do not init audio
        //0x80284670, 0x4E800020, //kill DSP IRQ handler (makes audio a VERY LOUD buzz)
        //0x8014d4f0, 0x4E800020, //do not update baddies
        //0x8014d650, 0x60000000, //do not run baddie seqs
        //0x8014a9f0, 0x4E800020, //disable baddie AI
        //0x8014a5fc, 0x4E800020, //something to do with jellyfish bug
        //0x8014a8bc, 0x38600000,
        //0x8006edcc, 0x4E800020,
        //0x80240608, 0x38000042, //don't change debug flags
        0x802543f8, 0x60000000, //don't change EXI1 settings, we're using it for debug
        //0x80240758, 0x60000000, //always call memInit
        //0x80241a14, 0x60000000, //don't syscall
        //0x80241a48, 0x60000000, //don't syscall
        0x802308e8, 0x60000000, //something to do with arwing audio streams.
            //crashes with it, seems to work fine without...

        //set some header values that apploader should set
        0x80000000, 0x47534145, //disc ID
        0x80000004, 0x30310000, //disc ID
        0x80000008, 0x01000000, //streaming params
        0x800000d0, 0x01000000, //ARAM size
        0x800000f0, 0x01800000, //simulated RAM size
        0x800030c0, 0xE2D383C1, //memory card is present

        //don't call __check_pad3 since it's not there anymore
        0x80003260, 0x60000000,

        //disable some DVD stuff
        0x802408fc, 0x60000000, //DVDInquiryAsync
        0x80015624, 0x4E800020, //dvdCheckError
        0x8004a900, 0x38000000, //don't set timeDelta to zero thinking
            //there's a DVD error message displayed.
            //this one is probably redundant but let's leave it...

        //__ARCheckSize probes ARAM to detect size, but this
        //clobbers MEM2, so disable it and set the size
        //manually instead.
        0x80250218, 0x4E800020, //__ARCheckSize
        0x803de01c, 0x01000000, //__ARSize
        0x803de020, 0x01000000, //__ARInternalSize
        0x803de024, 0x00000000, //__ARExpansionSize

        //use the Wii's config system
        0x8001fbe8, 0x60000000, //progressive scan prompt immediately closes
        0x800212c0, 0x60000000, //always go to progressive scan prompt
        0x8001fd64, 0x60000000, //progrssive scan confirm immediately closes
        0x8001fd44, 0x60000000, //don't show progressive scan confirm
        0x800e8564, 0x60000000, //don't override default widescreen setting
        0x800e8570, 0x60000000, //don't override default rumble setting
        0x800e8554, 0x60000000, //don't clear default savedata
            //XXX this might cause an issue on quitting from pause menu?

        0 //end of list
    };
    for(int i=0; patches[i]; i += 2) {
        (*(u32*)patches[i]) = patches[i+1];
        DCStoreRange((void*)patches[i], 32);
        ICInvalidateRange((void*)patches[i], 32);
    }

    //Nintendont does this. this is AR_REFRESH.
    //doesn't seem to make much difference though.
    // *(vu16*)0xCC00501A = 156;
}
