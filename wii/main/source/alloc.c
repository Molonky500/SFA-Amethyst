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

void* _my_sbrk_r( struct _reent *ptr, ptrdiff_t incr) {
    u32 level;
    _CPU_ISR_Disable(level);

    char *result = NULL;

    char *newEnd = _mem2_heap_start + incr;
    if(newEnd > (char*)_arena2_max) {
        ptr->_errno = ENOMEM;
        result = (char*)-1;
    }
    else if(newEnd < (char*)_arena2_min) {
        //freeing more than was allocated
        ptr->_errno = EINVAL;
        result = (char*)-1;
    }
    else {
        result = _mem2_heap_start;
        _mem2_heap_start += incr;

        //keep aligned to 32 bytes
        u32 pad = (u32)_mem2_heap_start & 0x1F;
        if(pad) _mem2_heap_start += 32 - pad;
    }

    //exiPrintf("sbrk(%08X, %d) => %08X, %d\n", (u32)ptr, incr,
    //    (u32)result, ptr->_errno);

    _CPU_ISR_Restore(level);
    return (void*)result;
}
