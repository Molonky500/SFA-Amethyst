/** Utility functions to hook game code.
 */
#include "main.h"

static u32 *trampoline = (u32*)0x80002b00;

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
    if(ABS(dist) >= 32*1024*1024) {
        //can't reach with a single branch
        uint32_t *code = (uint32_t*)addr;
        if((oldOp & ~0x03FFFFFF) == 0x48000000) {
            //replacing a b or bl
            if((u32)trampoline >= 0x80003000) {
                OSReport("Too many trampolines (%08X)\n", addr);
                while(1);
            }

            uint32_t relDest = (uint32_t)trampoline - addr;
            *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;

            OSReport("trampoline at %08X -> %08X op %08X -> %08X (bl=%d)\n",
                addr, trampoline, oldOp, *(uint32_t*)addr, isBl);
            *(trampoline++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(trampoline++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(trampoline++) = 0x7D8903A6; //mtspr CTR,r12
            *(trampoline++) = 0x4E800420; //bctr
            iCacheFlush((void*)&trampoline[-4], 32);
        }
        else {
            OSReport("direct long jump at %08X (bl=%d)\n",
                addr, isBl);
            *(code++) = 0x3D800000 | ((u32)target >> 16); //lis r12, aaaa
            *(code++) = 0x618C0000 | ((u32)target & 0xFFFF); //ori r12, r12, aaaa
            *(code++) = 0x7D8903A6; //mtspr CTR,r12
            *(code++) = 0x4E800420 | (isBl ? 1 : 0); //bctr or bctrl
        }
        iCacheFlush((void*)addr, 32);
    }
    else {
        //make b or bl opcode
        uint32_t relDest = (uint32_t)target - addr;
        *(uint32_t*)addr = (relDest & 0x03FFFFFC) | (isBl ? 1 : 0) | 0x48000000;
        OSReport("patch short jump at 0x%08X op 0x%08X (bl=%d)\n",
            addr, *(uint32_t*)addr, isBl);
        iCacheFlush((void*)addr, 32);
    }

    //decode original instruction and return original destination
    return addr + (oldOp & 0x03FFFFFC);
}
