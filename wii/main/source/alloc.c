#include "main.h"

GameHeap *gameHeaps = (GameHeap*)0x80340698;

void dumpGameHeaps() {
    exiPuts("H#|Entry|U| Address|    Size| EndAddr|AllocTag|      LR\n");
    for(int iHeap=0; iHeap<GAME_NUM_HEAPS; iHeap++) {
        GameHeap *heap = &gameHeaps[iHeap];
        for(int iEntry=0; iEntry<heap->avail; iEntry++) {
            GameHeapEntry *entry = &heap->data[iEntry];
            if(entry->size) { //ignore empty entries
                //size is not zero-padded
                exiPrintf("%2d|%5d|%c|%08X|%8X|%08X|%08X|%08X\n",
                    iHeap, iEntry,
                    entry->type == HEAP_ENTRY_TYPE_FREE ? ' ' : '*',
                    entry->loc, entry->size, entry->loc+entry->size,
                    entry->col, entry->unk14);
            }
        }
    }
}

bool checkAddrInheap(void *addr, u32 len) {
    GameHeapEntry *found = NULL;

    for(int iHeap=0; iHeap<GAME_NUM_HEAPS; iHeap++) {
        GameHeap *heap = &gameHeaps[iHeap];
        int iEntry = 0;
        while(iEntry >= 0) {
            GameHeapEntry *entry = &heap->data[iEntry];
            if(addr >= entry->loc
            && (addr+len) <= (entry->loc+entry->size)) {
                found = entry;
                if(entry->type != HEAP_ENTRY_TYPE_FREE) break;
            }
            iEntry = entry->next;
        }
    }
    if(!found) {
        exiPrintf("Addr %08X-%08X not found on heap\n",
            (u32)addr, (u32)addr+len);
        PANIC("Heap corruption detected!\n");
    }
    else if(found->type == HEAP_ENTRY_TYPE_FREE) {
        exiPrintf(" *** ERROR *** Addr %08X-%08X found in freed heap block %08X\n",
            (u32)addr, (u32)addr+len, found->loc);
        PANIC("Heap corruption detected!\n");
    }
    else return true;
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
