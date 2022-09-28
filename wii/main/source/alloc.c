#include "main.h"

GameHeap *gameHeaps = (GameHeap*)0x80340698;

bool checkAddrInheap(void *addr, u32 len) {
    for(int iHeap=0; iHeap<GAME_NUM_HEAPS; iHeap++) {
        GameHeap *heap = &gameHeaps[iHeap];
        for(int iBlock=0; iBlock<heap->avail; iBlock++) {
            GameHeapEntry *block = &heap->data[iBlock];
            if(block->type == 0) continue; //free block
            if((u32)addr       >=  (u32)block->loc
            && (u32)(addr+len) <= ((u32)block->loc + block->size)) {
                return true;
            }
        }
    }
    exiPrintf("Addr %08X-%08X not found on heap\n",
        (u32)addr, (u32)addr+len);
    return false;
}
