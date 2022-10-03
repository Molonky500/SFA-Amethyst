#ifndef __OSBOOTINFO_H__
#define __OSBOOTINFO_H__

typedef struct {
    DVDDiskID  diskId;
    u32        magic;
    u32        version;
    u32        memorySize;
    u32        consoleType;
    void*      arenaLo;         // if non NULL, overrides __ArenaLo
    void*      arenaHi;         // if non NULL, overrides FSTLocation
    void*      FSTLocation;     // Start address of "FST area"
    u32        FSTMaxLength;    // Length of "FST area"
} OSBootInfo;

#endif //__OSBOOTINFO_H__
