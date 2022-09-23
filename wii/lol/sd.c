#include "main.h"

fat32_mbr mbr;
static struct {
    bool open;
    myDirent ent;
    u32 pos;
} _files[SD_MAX_FILES_OPEN];

int initSd() {
    memset(_files, 0, sizeof(_files));
    exiPuts("SD init...\n");
    if(!sdio_Startup()) {
        exiPuts("SD init fail\n");
        return -1;
    }

    //lazy: scan sectors to find MBR
    int err;
    uint64_t iSector;
    while(1) {
        err = fatGetMBR(iSector, &mbr, 1000);
        if(err != -EILSEQ) return;
        iSector++;
    }

    char msg[1024];
    if(!err) {
        snprintf(msg, sizeof(msg), "found FAT MBR at sector %lld\n",
            iSector);
    }
    else {
        snprintf(msg, sizeof(msg), "find FAT MBR failed: %d\n", err);
    }
    exiPuts(msg);

    return err;
}

int sdReadBlock(sector, out, timeout) {
    bool r = sdio_ReadSectors(sector, 1, out);
    return r ? 0 : -1;
}

static u32 sdFindFile(const char *path, u32 iEnt) {
    myDirent ent;

    while(1) {
        int err = fatReadDir(&mbr, iEnt, &ent, 1000);
        if(err) {
            char msg[1024];
            snprintf(msg, sizeof(msg), "sdFindFile err: %d\n", err);
            exiPuts(msg);
            exiPuts(path);
            exiPuts("\r\n");
            return 0;
        }
        else if(!strcmp(ent.name, path)) break;
    }

    char msg[1024];
    snprintf(msg, sizeof(msg), "sdFindFile(%s) => %d\n", path, iEnt);
    exiPuts(msg);

    return iEnt;
}

static u32 sdFindPath(const char *path) {
    u32 iEnt = 0;
    char name[1024];
    int iPath = 0;
    int iName = 0;
    char c;
    while((c = path[iPath++])) {
        if(c == '/') {
            if(iName > 0) {
                iEnt = sdFindFile(name, iEnt);
                iName = 0;
                if(iEnt == 0) break;
            }
        }
        else if(iName < sizeof(name)) {
            name[iName++] = c;
            name[iName] = 0;
        }
    }

    return iEnt;
}

int sdOpen(const char *path) {
    u32 iEnt = sdFindPath(path);
    if(!iEnt) return -ENOENT;

    int fd = 0;
    while(1) {
        if(fd >= SD_MAX_FILES_OPEN) return -EMFILE;
        if(!_files[fd].open) break;
        fd++;
    }

    fatReadDir(&mbr, iEnt, &_files[fd].ent, 1000);
    _files[fd].pos = 0;
    _files[fd].open = true;
    return fd;
}

int sdClose(int fd) {
    _files[fd].open = false;
}

int sdSeek(int fd, int offs, int whence) {
    switch(whence) {
        case 0: //SEEK_SET
            _files[fd].pos = offs;
            break;
        case 1: //SEEK_CUR
            _files[fd].pos += offs;
            break;
        case 2: //SEEK_END
            _files[fd].pos = _files[fd].ent.size - offs;
            break;
        default: return -EINVAL;
    }
    return 0;
}

u32 sdTell(int fd) {
    return _files[fd].pos;
}

u32 sdGetFileSize(int fd) {
    return _files[fd].ent.size;
}

int sdRead(int fd, uint32_t size, void *dest) {
    int r = fatReadFile(&mbr, &_files[fd].ent, _files[fd].pos,
        size, dest, 1000);
    if(r >= 0) _files[fd].pos += r;
    return r;
}
