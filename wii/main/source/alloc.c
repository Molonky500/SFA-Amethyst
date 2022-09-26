#include "main.h"

//probably unneccesary as this malloc is already threadsafe
mutex_t allocMutex;

/*
void* alloc_hook(u32 size, u32 tag, const char *name) {
    switchToOgc();
    //LWP_MutexLock(allocMutex);
    u32 irq = IRQ_Disable();

    void *buf = heap_alloc(&heapMem1, size+32);
    if(!buf) {
        IRQ_Restore(irq);
        //LWP_MutexUnlock(allocMutex);
        exiPrintf(" *** ERROR *** heap_alloc(%d, %08X, %s) FAILED\n",
            size, tag, name);
        switchToGame();
        return NULL;
    }
    *(u32*)buf = tag;
    *(char*)(buf+4) = name;
    *(u32*)(buf+8) = __builtin_extract_return_addr(__builtin_return_address(0));

    IRQ_Restore(irq);
    //LWP_MutexUnlock(allocMutex);
    exiPrintf("malloc(%d, %08X, %s) => %08X\n",
        size, tag, name, buf);
    switchToGame();
    return buf + 32;
}

void free_hook(void *addr) {
    switchToOgc();
    //LWP_MutexLock(allocMutex);
    exiPrintf("free(%08X)\n", addr - 32);
    u32 irq = IRQ_Disable();

    heap_free(&heapMem1, addr - 32);

    IRQ_Restore(irq);
    //LWP_MutexUnlock(allocMutex);
    switchToGame();
}
*/

typedef struct {
    void *loc;
    int	size;
    u16 type;
    s16 prev;  //idx of prev block
    s16 next;  //idx of next block
    s16 stack; //usually (not always) idx of this block
    u32 col;   //this is what the game calls this field.
    int unk14; //not unused
    int mmUniqueIdent;
} GameHeapEntry;

typedef struct {
    u32 dataSize; //total size
    u32 size;     //used size
    u32 avail;    //total blocks
    u32 used;     //used blocks
    GameHeapEntry *data;
} GameHeap;

bool checkAddrInheap(void *addr, u32 len) {
    GameHeap *heaps = (GameHeap*)0x80340698;
    for(int iHeap=0; iHeap<4; iHeap++) {
        GameHeap *heap = &heaps[iHeap];
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

void alloc_init() {
    //LWP_MutexInit(&allocMutex, false);

    /*u32 start = 0x803fa480;
    u32 addr  = start;
    u32 size  = 0x817ea240 - start;
    memset((void*)start, 0, size);
    memset(&heapMem1, 0, sizeof(heap_t));

    for(int i = 0; i < BIN_COUNT; i++) {
        heapMem1.bins[i] = addr;
        memset(heapMem1.bins[i], 0, sizeof(bin_t));
        //memset(heapMem2->bins[i], 0, sizeof(bin_t));
        addr += sizeof(bin_t);
    }
    init_heap(&heapMem1, (long)start, size);
    //init_heap(&heapMem2, (long)start[1], size[1]);*/
}
