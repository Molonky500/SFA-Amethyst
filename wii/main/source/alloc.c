#include "main.h"
#include "heap.h"
#include "llist.h"

//probably unneccesary as this malloc is already threadsafe
mutex_t allocMutex;
heap_t heapMem1; //, heapMem2;

//https://github.com/CCareaga/heap_allocator
uint offset = 8;
void init_heap(heap_t *heap, long start, long size) {
    node_t *init_region = (node_t *) start;
    init_region->hole = 1;
    init_region->size = size - sizeof(node_t) - sizeof(footer_t);

    create_foot(init_region);

    add_node(heap->bins[get_bin_index(init_region->size)], init_region);

    heap->start = (void *) start;
    heap->end   = (void *) (start + size);
}

void *heap_alloc(heap_t *heap, size_t size) {
    uint index = get_bin_index(size);
    bin_t *temp = (bin_t *) heap->bins[index];
    node_t *found = get_best_fit(temp, size);

    while (found == NULL) {
        if (index + 1 >= BIN_COUNT)
            return NULL;

        temp = heap->bins[++index];
        found = get_best_fit(temp, size);
    }

    if ((found->size - size) > (overhead + MIN_ALLOC_SZ)) {
        node_t *split = (node_t *) (((char *) found + sizeof(node_t) + sizeof(footer_t)) + size);
        split->size = found->size - size - sizeof(node_t) - sizeof(footer_t);
        split->hole = 1;

        create_foot(split);

        uint new_idx = get_bin_index(split->size);

        add_node(heap->bins[new_idx], split);

        found->size = size;
        create_foot(found);
    }

    found->hole = 0;
    remove_node(heap->bins[index], found);

    node_t *wild = get_wilderness(heap);
    if (wild->size < MIN_WILDERNESS) {
        uint success = expand(heap, 0x1000);
        if (success == 0) {
            return NULL;
        }
    }
    else if (wild->size > MAX_WILDERNESS) {
        contract(heap, 0x1000);
    }

    found->prev = NULL;
    found->next = NULL;
    return &found->next;
}

void heap_free(heap_t *heap, void *p) {
    bin_t *list;
    footer_t *new_foot, *old_foot;

    node_t *head = (node_t *) ((char *) p - offset);
    if (head == (node_t *) (uintptr_t) heap->start) {
        head->hole = 1;
        add_node(heap->bins[get_bin_index(head->size)], head);
        return;
    }

    node_t *next = (node_t *) ((char *) get_foot(head) + sizeof(footer_t));
    footer_t *f = (footer_t *) ((char *) head - sizeof(footer_t));
    node_t *prev = f->header;

    if (prev->hole) {
        list = heap->bins[get_bin_index(prev->size)];
        remove_node(list, prev);

        prev->size += overhead + head->size;
        new_foot = get_foot(head);
        new_foot->header = prev;

        head = prev;
    }

    if (next->hole) {
        list = heap->bins[get_bin_index(next->size)];
        remove_node(list, next);

        head->size += overhead + next->size;

        old_foot = get_foot(next);
        old_foot->header = 0;
        next->size = 0;
        next->hole = 0;

        new_foot = get_foot(head);
        new_foot->header = head;
    }

    head->hole = 1;
    add_node(heap->bins[get_bin_index(head->size)], head);
}

uint expand(heap_t *heap, size_t sz) {
    return 0;
}

void contract(heap_t *heap, size_t sz) {
    return;
}

uint get_bin_index(size_t sz) {
    uint index = 0;
    sz = sz < 4 ? 4 : sz;

    while (sz >>= 1) index++;
    index -= 2;

    if (index > BIN_MAX_IDX) index = BIN_MAX_IDX;
    return index;
}

void create_foot(node_t *head) {
    footer_t *foot = get_foot(head);
    foot->header = head;
}

footer_t *get_foot(node_t *node) {
    return (footer_t *) ((char *) node + sizeof(node_t) + node->size);
}

node_t *get_wilderness(heap_t *heap) {
    footer_t *wild_foot = (footer_t *) ((char *) heap->end - sizeof(footer_t));
    return wild_foot->header;
}


//hooks and such
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
    LWP_MutexInit(&allocMutex, false);

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
