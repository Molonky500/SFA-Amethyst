#include "main.h"

#if LOG_OBJS

typedef struct PACKED {
    u32 time;
    float playerPos[3];
    s8 mapLayer;
    s16 mapId;
    s8 _pad;
    u32 flags;
    ObjDef def;
    u8 params[128];
} ObjLogEntry;
ObjLogEntry objLog[MAX_OBJ_TYPE+1];
static bool logIsInit = false;
static bool logNeedsWrite = false;
static u32  logWriteDelay = 0;
static char objLogPath[1024];

void objLogInit() {
    if(logIsInit) return;
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return;
    sprintf(objLogPath, "%s/objlog.bin", wii->gameRootDir);

    memset(objLog, 0, sizeof(objLog));
    FILE *file = wii->fopen(objLogPath, "rb");
    if(!file) {
        DPRINT("No obj log\r\n");
        logIsInit = true;
        return;
    }
    DPRINT("Reading obj log\r\n");
    wii->fread(objLog, sizeof(ObjLogEntry), MAX_OBJ_TYPE+1, file);
    wii->fclose(file);
    logIsInit = true;
    DPRINT("Obj log init OK\r\n");
}

void objLogUpdate(ObjDef *def,uint flags,int mapId,s16 objNo) {
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!(wii && wii->fopen)) return;
    objLogInit();

    ObjLogEntry *entry = &objLog[objNo];
    if(entry->time) return;
    entry->time = curTimeStamp;
    entry->mapLayer = curMapLayer;
    entry->flags = flags;
    entry->mapId = mapId;
    memcpy(&entry->def, def, sizeof(ObjDef)+128);

    if(PTR_VALID(pPlayer)) {
        entry->playerPos[0] = pPlayer->pos.pos.x;
        entry->playerPos[1] = pPlayer->pos.pos.y;
        entry->playerPos[2] = pPlayer->pos.pos.z;
    }
    else {
        entry->playerPos[0] = 0;
        entry->playerPos[1] = 0;
        entry->playerPos[2] = 0;
    }
    logNeedsWrite = true;
}

void objLogWrite() {
    if(!logNeedsWrite) return;
    if(!IS_WII) return;
    if(logWriteDelay > 0) {
        logWriteDelay--;
        return;
    }
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!(wii && wii->fopen)) return;

    FILE *file = wii->fopen(objLogPath, "wb");
    if(!file) {
        OSReport("ERR: Failed to open obj log to write\r\n");
        return;
    }
    DPRINT("Writing obj log\r\n");
    wii->fwrite(objLog, sizeof(ObjLogEntry), MAX_OBJ_TYPE+1, file);
    wii->fclose(file);
    logNeedsWrite = false;
    logWriteDelay = 3600; //60 frames * 60 seconds = 1 minute
}
#endif

ObjInstance* loadCharacter_hook(ObjDef *def,uint flags,int mapId,s16 objNo,
ObjInstance *heldBy) {
    #if LOG_OBJS
        s32 defNo = def->objType;
        s32 realObjType = defNo;
        if((flags & 2) == 0) {
            if(realObjType > MAX_OBJ_TYPE) {
                return NULL;
            }
            s16 *objIndex = dataFileBuffers[FILE_OBJINDEX_BIN];
            realObjType = objIndex[realObjType];
        }
        objLogUpdate(def, flags, mapId, objNo);
    #endif
    return loadCharacter(def, flags, mapId, objNo, heldBy);
}

bool isObjectEnabled(ObjInstance *obj) {
    if(!PTR_VALID(obj)) return false;
    switch(obj->file->dll_id) {
        case 0x174: //CCriverflow
            return *((u8*)obj->state);

        //XXX more
        default: return true;
    }
}

void objHooksinit() {
    hookBranch(0x8001f5dc, loadCharacter_hook, 1); //loadAsset
    hookBranch(0x8002b610, loadCharacter_hook, 1); //loadObjectAtObject
    hookBranch(0x8002bb94, loadCharacter_hook, 1); //mapSetupPlayer
    hookBranch(0x8002e000, loadCharacter_hook, 1); //objSetupObject
}
