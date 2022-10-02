#define DVD_DEBUG 0

#if DVD_DEBUG
#define DVD_DPRINT exiPrintf
#else
#define DVD_DPRINT(...)
#endif

//how much to read at once (doesn't need to be the same
//as the real sector size of a DVD)
//#define DVD_SECTOR_SIZE 2048
#define DVD_SECTOR_SIZE (32*1024*1024) //gotta go fast
//#define DVD_SECTOR_SIZE (327680) //about real speed
//since we sleep the DVD thread between reads, using the
//real sector size is extremely slow.

#define DVD_MAX_MSGS 1024
#define DVD_THREAD_PRIO 4 //31=lowest 0=highest
#define DVD_MAX_OPEN_FILES 256
#define DVD_ALARM_PERIOD OSMillisecondsToTicks(1)
#define DVD_THREAD_STACK_SIZE 65536

typedef void (*DVDCallback)(s32 result, void* fileInfo);
#define DVD_BUSY (*(vu8*)0x803dc950)

typedef struct DVDDiskID DVDDiskID;

struct DVDDiskID
{
    char      gameName[4];
    char      company[2];
    u8        diskNumber;
    u8        gameVersion;
    u8        streaming;
    u8        streamingBufSize; // 0 = default
    u8        padding[22];      // 0's are stored
};

typedef struct DVDCommandBlock DVDCommandBlock;

typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);

struct DVDCommandBlock
{
    DVDCommandBlock* next;
    DVDCommandBlock* prev;
    u32              command;
    s32              state;
    u32              offset;
    u32              length;
    void*            addr;
    u32              currTransferSize;
    u32              transferredSize;
    DVDDiskID*       id;
    DVDCBCallback    callback;
    void*            userData;
};

typedef struct DVDFileInfo DVDFileInfo;

struct DVDFileInfo {
    DVDCommandBlock cb;
    u32 startAddr; //disk address of file
    u32 length; //file size in bytes
    DVDCallback callback; //DVDCallback
};

typedef struct {
    DVDFileInfo *info;
    FILE *file;
    #if DVD_DEBUG
        char path[256];
    #endif
} HackDvdOpenFile;

typedef enum {
    DVDCMD_SHUTDOWN, //stop the DVD thread
    DVDCMD_READ,
} HackDvdCmd;

typedef struct {
    u8 cmd;
    u32 id;
    union {
        struct { //DVDCMD_READ
            HackDvdOpenFile *file;
            void *addr;
            int length;
            uint offset;
            DVDCallback callback;
            int prio;
            bool async;
        } read;
    };
} HackDvdMsg;
