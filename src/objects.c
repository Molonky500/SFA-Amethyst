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

//XXX add some way to change these

//allow the player to run around while time is stopped
//for everything else. fun little thing.
bool gPlayerMoveWhileTimeStopped = false;

//scale movement speeds
float gPlayerTimeScale = 1.0f;
float gBaddieTimeScale = 1.0f;
float gBikeTimeScale   = 1.0f;

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

void objSendMsg_hook(ObjInstance *target, uint msgId,
ObjInstance *sender, uint param) {
    char tgtName[16], senderName[16];
    getObjName(tgtName,    target);
    getObjName(senderName, sender);
    ObjMsgQueue *queue = target ? target->msgQueue : NULL;
    int count=0, size=0;
    if(queue) { count = queue->count; size = queue->size; }

    OSReport("objSendMsg(to 0x%X %s, from 0x%X %s, ID 0x%08X param 0x%08X) %d/%d\n",
        target, tgtName,
        sender, senderName,
        msgId, param, count+1, size);
}
void _objSendMsg_hook(void);
__asm__(
    "_objSendMsg_hook:\n"
    ASM_FUNC_START(0x100)
    "bl objSendMsg_hook\n"
    ASM_FUNC_END(0x100)
    "stwu 1, -16(1)\n" //replaced
    "lis   9, 0x8003\n"
    "ori   9, 9, 0x78c8\n"
    "mtctr 9\n"
    "bctr"
);

void objSendMsgAll_hook(int objId, uint flags, ObjInstance *sender,
uint msgId, uint param) {
    char senderName[16];
    getObjName(senderName, sender);

    OSReport("objSendMsgAll(from 0x%X %s, objId 0x%08X, ID 0x%08X param 0x%08X flags 0x%X)\n",
        sender, senderName, objId, msgId, param, flags);
}
void _objSendMsgAll_hook(void);
__asm__(
    "_objSendMsgAll_hook:\n"
    ASM_FUNC_START(0x100)
    "bl objSendMsgAll_hook\n"
    ASM_FUNC_END(0x100)
    "stwu 1, -16(1)\n" //replaced
    "lis   9, 0x8003\n"
    "ori   9, 9, 0x76dc\n"
    "mtctr 9\n"
    "bctr"
);

void objSendMsgNear_hook(u16 defNo, uint flags, ObjInstance *sender,
uint msgId, uint param, float maxDistance) {
    char senderName[16];
    getObjName(senderName, sender);

    OSReport("objSendMsgNear(from 0x%X %s, defno 0x%08X, ID 0x%08X param 0x%08X flags 0x%X dist %f)\n",
        sender, senderName, defNo, msgId, param, flags, maxDistance);
}
void _objSendMsgNear_hook(void);
__asm__(
    "_objSendMsgNear_hook:\n"
    ASM_FUNC_START(0x100)
    "bl objSendMsgNear_hook\n"
    ASM_FUNC_END(0x100)
    "stwu 1, -16(1)\n" //replaced
    "lis   9, 0x8003\n"
    "ori   9, 9, 0x75a0\n"
    "mtctr 9\n"
    "bctr"
);

void objUpdate_hook(ObjInstance *obj) {
    switch(obj->defNo) {
        case ObjDefEnum_Sabre:
        case ObjDefEnum_Krystal:
        case ObjDefEnum_staff:
        case ObjDefEnum_ARWArwing: {
            float prevDelta = timeDelta;
            timeDelta *= gPlayerTimeScale;
            objUpdate(obj);
            timeDelta = prevDelta;
            break;
        }

        default: {
            ObjInstance *mount = NULL;
            ObjInstance *heldBy = NULL;
            if(pPlayer) {
                heldBy = pPlayer->heldBy;
                mount  = pPlayer->state ? *(ObjInstance**)(pPlayer->state + 0x7F0) : NULL;
            }
            u8 prevStop = timeStop;
            u32 prevStopFrame = timeStoppedThisFrame;
            if(gPlayerMoveWhileTimeStopped && obj != mount
            && obj != heldBy) {
                //freeze this object
                timeStop = 3; //stop physics and animations
                timeStoppedThisFrame = 3;
            }
            switch(obj->catId) {
                case ObjCatId_baddie: {
                    float prevDelta = timeDelta;
                    timeDelta *= gBaddieTimeScale;
                    objUpdate(obj);
                    timeDelta = prevDelta;
                    break;
                }
                case ObjCatId_bike: {
                    //this can make the bikes faster, but it's not
                    //the same as the turbo function (which changes
                    //the movement scale); this acts like fast-forward,
                    //so the physics aren't changed.
                    float prevDelta = timeDelta;
                    timeDelta *= gBikeTimeScale;
                    objUpdate(obj);
                    timeDelta = prevDelta;
                    break;
                }
                default: objUpdate(obj);
            }
            timeStop = prevStop;
            timeStoppedThisFrame = prevStopFrame;
        }
    }
}

void objHooksinit() {
    hookBranch(0x8001f5dc, loadCharacter_hook, 1); //loadAsset
    hookBranch(0x8002b610, loadCharacter_hook, 1); //loadObjectAtObject
    hookBranch(0x8002bb94, loadCharacter_hook, 1); //mapSetupPlayer
    hookBranch(0x8002e000, loadCharacter_hook, 1); //objSetupObject

    hookBranch(0x8002e6a0, objUpdate_hook, 1);
    hookBranch(0x8002e774, objUpdate_hook, 1);
    hookBranch(0x80036990, objUpdate_hook, 1);
    hookBranch(0x8002e720, objUpdate_hook, 1);
    hookBranch(0x8002e714, objUpdate_hook, 1);
    hookBranch(0x8002e678, objUpdate_hook, 1);

    hookBranch(0x800378c4, _objSendMsg_hook, 0);
    hookBranch(0x800376d8, _objSendMsgAll_hook, 0);
    hookBranch(0x8003759c, _objSendMsgNear_hook, 0);
}
