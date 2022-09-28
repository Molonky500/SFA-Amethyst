#define GAME_NUM_HEAPS 4

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

extern GameHeap *gameHeaps;
