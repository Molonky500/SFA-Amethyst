#include "main.h"

GameHeap *gameHeaps = (GameHeap*)0x80340698;

void dumpGameHeaps() {
    for(int iHeap=0; iHeap<heapCount; iHeap++) {
        exiPuts("H#|Entry|U| Address|    Size| EndAddr|AllocTag|      LR\n");
        GameHeap *heap = &gameHeaps[iHeap];
        for(int iEntry=0; iEntry<heap->avail; iEntry++) {
            GameHeapEntry *entry = &heap->data[iEntry];
            if(entry->size) { //ignore empty entries
                //size is not zero-padded
                printf("%2d|%5d|%c|%08X|%8X|%08X|%08X|%08X\n",
                    iHeap, iEntry,
                    entry->type == HEAP_ENTRY_TYPE_FREE ? ' ' : '*',
                    entry->loc, entry->size, entry->loc+entry->size,
                    entry->col, entry->unk14);
            }
        }
    }
    exiPuts("--- END OF HEAP DUMP ---\n");
}

bool checkAddrInheap(void *addr, u32 len) {
    GameHeapEntry *found = NULL;

    for(int iHeap=0; iHeap<heapCount; iHeap++) {
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

void* _sbrk_r( struct _reent *ptr, ptrdiff_t incr) {
    u32 level;
    _CPU_ISR_Disable(level);

    char *result = NULL;

    char *newEnd = _mem2_heap_start + incr;
    if(newEnd > (char*)_arena2_max) {
        exiPrintf("sbrk(%p, 0x%x): ENOMEM\n", ptr, incr);
        ptr->_errno = ENOMEM;
        result = (char*)-1;
    }
    else if(newEnd < (char*)_arena2_min) {
        //freeing more than was allocated
        exiPrintf("sbrk(%p, 0x%x): EINVAL\n", ptr, incr);
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

void initHeaps_hook() {
    //replaces game's initHeaps() if there's enough memory

    u32 newSize = 8*1024*1024;
    exiPrintf("Allocating %d bytes for game\r\n", newSize);
    void *gameExtraHeap = malloc(newSize);
    if(gameExtraHeap) {
        exiPrintf("Allocated extra game heap at 0x%X - 0x%X\r\n", gameExtraHeap,
            (u32)gameExtraHeap + newSize);
    }
    else {
        exiPuts("Failed to alloc extra heap for game\r\n");
        void (*origFunc)() = 0x80023f9c;
        origFunc();
        return;
    }
    DCFlushRange(gameExtraHeap,newSize);

    //by adding another heap we overwrite some variables that
    //are used by some debug prints, so disable them
    *((u32*)0x80023764) = 0x3800F0C5; //in mmFreeTick
    *((u32*)0x80022d58) = 0x4E800020; //printHeapStats

    *(u8*)0x803dcb42 = 0; //heapCount = 0;
    OSHeapHandle heapHandle_803dc530 = *(OSHeapHandle*)0x803dc530;

    //get available size for heap0 (same as game does)
    void *arenaStart = getArenaStart();
    void *arenaEnd = OSGetArenaHi();
    u32 size0 = (int)arenaEnd + (-0x6c0720 - (int)arenaStart);

    struct {
        u32 size;
        u32 nBlocks;
    } heaps[] = {
        {   size0,  250}, //Heap 0: for large allocations (default for >= 0x3000)
        {    1773,    0}, //save data (idk why game puts it between these)
        { 1835008,  850}, //Heap 1: for medium allocations (default for >= 0x400)
        {  655264,  850}, //Heap 2: for small  allocations (default for < 0x400)
        { 4587424,  580}, //Heap 3: for some special cases (certain audio, textures, ?)
        {0, 0} //end
    };

    for(int i=0; heaps[i].size; i++) {
        u32 size = heaps[i].size;
        u32 nBlocks = heaps[i].nBlocks;
        void *addr = OSAllocFromHeap(heapHandle_803dc530,size);
        if(!addr) {
            exiPrintf(" *** ERROR *** OSAllocFromHeap(%d) failed\r\n", size);
            PANIC("Heap alloc failed");
        }
        if(nBlocks) {
            exiPrintf("Heap size %8d, %4d blocks: 0x%X - 0x%X\r\n", size, nBlocks, addr,
                (u32)addr + size);
            //DCFlushRange(addr,size);
            heapInit(addr,size,nBlocks);
        }
        else {
            exiPrintf("Save buffer: 0x%X\r\n", addr);
            SaveGame **pCurSaveGame = (SaveGame**)0x803dd498;
            SaveGame **pSaveStart   = (SaveGame**)0x803dcafc;
            *pCurSaveGame = (SaveGame *)addr;
            *pSaveStart = (SaveGame *)(addr+0x6EC);
        }
    }
    heapInit(gameExtraHeap,newSize,1000);

    //duplicate game logic
    *(u32*)0x803dcb18 = heaps[0].size; //heapSize_803dcb18 = size;

    u32 *mmSetDelay_count = (u32*)0x803dcb14;
    *mmSetDelay_count = *mmSetDelay_count + 1;

    int *mm_delay = (int*)0x803dcb3c;
    *mm_delay = 2;

    s16 *mmFreeQueueLen = (s16*)0x803dcb40;
    *mmFreeQueueLen = 0;

    exiPuts("alloc setup OK\r\n");
}
