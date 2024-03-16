#ifndef __GAMEHEAP_H__
#define __GAMEHEAP_H__

#define GAME_NUM_HEAPS 5

typedef enum {
    HEAP_ENTRY_TYPE_FREE = 0, //free block
    HEAP_ENTRY_TYPE_RAM  = 1, //RAM block
    //only types 1 and 4 can be freed.
    //the game seems to only use types 0 and 1.
    //presumably the others would have been for different
    //memory regions on the N64, such as cartridge ROM,
    //or for ARAM on the GC.
    HEAP_ENTRY_TYPE_RAM2 = 4, //unknown, but can be freed
} GameHeapEntryType;

typedef struct {
    void *loc;
    int	size;
    u16 type;
    s16 prev;  //idx of prev block
    s16 next;  //idx of next block
    s16 stack; //usually (not always) idx of this block
    u32 col;   //this is what the game calls this field.
    int unk14; //set but never read
    int mmUniqueIdent;
} GameHeapEntry;

typedef struct {
    u32 dataSize; //total size
    u32 size;     //used size
    u32 avail;    //total blocks
    u32 used;     //used blocks
    GameHeapEntry *data;
} GameHeap;

extern GameHeap *gameHeaps;

#endif //__GAMEHEAP_H__
