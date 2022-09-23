#define SD_MAX_FILES_OPEN 8
#define SDCARD_DEBUG_PRINT 1
#define SD_BLOCK_SIZE 512

typedef struct PACKED {
    uint8_t  jumpCode[3];
    char     oemName[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t  numFats; //# copies of FAT
    uint16_t maxRootDirEnts; //N/A for FAT32
    uint16_t numSmallSectors; //# sectors < 32MB, N/A for FAT32
    uint8_t  mediaDescriptor; //0xF8=hard disk
    uint16_t sectorsPerFat; //FAT32 uses sectorsPerFat32 instead
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    uint32_t numHiddenSectors;
    uint32_t numSectors;
    uint32_t sectorsPerFat32;
    uint16_t flags;
    uint16_t version;
    uint32_t rootCluster; //cluster # of start of root dir
    uint16_t fsInfoSector;
    uint16_t mbrBackupSector;
    uint8_t  reserved[12];
    uint16_t driveNum;
    uint8_t  extSig; //extended signature, 0x29
    uint32_t serial;
    char     volName[11];
    char     fatName[8];
    union PACKED {
        uint8_t  bootCode[420]; //executable code for DOS
        uint64_t _my_startSector; //we'll use this to store the sector
            //that the MBR is at
    };
    uint16_t mbrSig; //MBR signature: 0x55 0xAA
} fat32_mbr;

typedef struct PACKED {
    uint32_t signature; //0x52526141 "RRaA"
    uint8_t  padding[480];
    uint32_t fsInfoSig; //0x72724161 "rrAa"
    uint32_t numFreeClusters;
    uint32_t lastUsedCluster;
    uint8_t  reserved[14];
    uint16_t mbrSig;
} fat32_fsinfo;

#define FAT_ATTR_READONLY     0x01
#define FAT_ATTR_HIDDEN       0x02
#define FAT_ATTR_SYSTEM       0x04
#define FAT_ATTR_VOLUME_LABEL 0x08
#define FAT_ATTR_DIRECTORY    0x10
#define FAT_ATTR_ARCHIVE      0x20
#define FAT_ATTR_DEVICE       0x40 //not used on disk
//0x80: reserved
//attribute value 0x0F marks long file names

typedef struct PACKED {
    char shortName[8]; //space padded
        //first byte has special meanings:
        //0x00: last used entry
        //0x05: first character is 0xE5
        //0xE5: deleted entry
    char shortExt[3]; //file extension, space padded
    uint8_t attributes; //FAT_ATTR_*
    uint8_t extAttributes; //various OS-specific uses
    union {
        uint8_t createCsec; //creation time, 10ms units, 0 to 199
        char delName0; //first char of deleted file name
    };
    uint16_t createTime; //time of day
        //bits 15-11: hour
        //bits 10- 5: minute
        //bits  4- 0: second / 2
    uint16_t createDate; //date
        //bits 15- 9: year (+1980)
        //bits  8- 5: month
        //bits  4- 0: day
    uint16_t accessDate;
    uint16_t startClusterHi; //high 2 bytes of start cluster
    uint16_t modifyTime;
    uint16_t modifyDate;
    uint16_t startClusterLo; //low 2 bytes of start cluster
    uint32_t size; //file size in bytes
} fat32_dirent;

typedef struct PACKED {
    uint8_t seq; //sequence number
    uint16_t name0[5]; //five UCS-2 characters
    uint8_t attributes; //always 0x0F (corresponds with fat32_dirent.attributes)
    uint8_t type; //always 0 (never used)
    uint8_t checksum;
    uint16_t name1[6];
    uint16_t cluster; //always 0 (corresponds with startClusterLo)
    uint16_t name2[2];
} vfat_lfn; //long file name entry
//this is stored as a directory entry for legacy reasons

typedef struct {
    char name[2048];
    uint32_t attributes;
    uint32_t createTime; //UNIX timestamp
    uint32_t accessTime; //UNIX timestamp
    uint32_t modifyTime; //UNIX timestamp
    uint32_t size; //file size in bytes
    uint64_t cluster; //FS-specific start cluster/file ID
} myDirent;

void fatDecodeDate(uint16_t date, uint16_t *year, uint8_t *month, uint8_t *day);
void fatDecodeTime(uint16_t time, uint8_t *hour, uint8_t *minute, uint8_t *second);
int fatGetMBR(uint64_t sector, fat32_mbr *out, uint32_t timeout);
int fatGetFsInfo(fat32_mbr *mbr, fat32_fsinfo *out, uint32_t timeout);
int fatGetNextCluster(fat32_mbr *mbr, int cluster, uint32_t timeout);
int fatGetDirEntry(fat32_mbr *mbr, uint32_t idx, fat32_dirent *out, uint32_t timeout);
int fatReadDir(fat32_mbr *mbr, int idx, myDirent *out, uint32_t timeout);
int fatReadFile(fat32_mbr *mbr, myDirent *file, uint32_t offset, uint32_t size, void *out, uint32_t timeout);
int fatGetInfo(uint64_t sector, uint32_t timeout);
