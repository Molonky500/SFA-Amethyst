#include "main.h"
#include "revolution/os.h"

#if LOG_DLLS
typedef struct PACKED {
    u16 param;
    u32 time;
    float playerPos[3];
    s8 mapLayer;
    void *ra;
} DllLogEntry;

DllLogEntry dllLog[NUM_DLLS];
static bool logIsInit = false;
static bool logNeedsWrite = false;
static char dllLogPath[1024];

void dllLogInit() {
    if(logIsInit) return;
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return;
    DPRINT("game root = %s\n", wii->gameRootDir);
    sprintf(dllLogPath, "%s/dlllog.bin", wii->gameRootDir);

    /*DPRINT("fopen     = %p\r\n", wii->fopen);
    DPRINT("fclose    = %p\r\n", wii->fclose);
    DPRINT("fread     = %p\r\n", wii->fread);
    DPRINT("fwrite    = %p\r\n", wii->fwrite);
    DPRINT("opendir   = %p\r\n", wii->opendir);
    DPRINT("closedir  = %p\r\n", wii->closedir);
    DPRINT("readdir   = %p\r\n", wii->readdir);
    DPRINT("rewinddir = %p\r\n", wii->rewinddir);
    DPRINT("seekdir   = %p\r\n", wii->seekdir);
    DPRINT("telldir   = %p\r\n", wii->telldir);
    DPRINT("stat      = %p\r\n", wii->stat);*/

    memset(dllLog, 0, sizeof(dllLog));
    FILE *file = wii->fopen(dllLogPath, "rb");
    if(!file) {
        DPRINT("No DLL log\r\n");
        logIsInit = true;
        return;
    }
    DPRINT("Reading DLL log\r\n");
    wii->fread(dllLog, sizeof(DllLogEntry), NUM_DLLS, file);
    wii->fclose(file);
    logIsInit = true;
    DPRINT("DLL log init OK\r\n");
}

void dllLogUpdate(int dll_id, u16 param, void *ra) {
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!(wii && wii->fopen)) return;
    dllLogInit();

    DllLogEntry *entry = &dllLog[dll_id];
    if(entry->time) return;
    entry->time = curTimeStamp;
    entry->param = param;
    entry->mapLayer = curMapLayer;
    entry->ra = ra;

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

void dllLogWrite() {
    if(!logNeedsWrite) return;
    if(!IS_WII) return;
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!(wii && wii->fopen)) return;

    FILE *file = wii->fopen(dllLogPath, "wb");
    if(!file) {
        OSReport("ERR: Failed to open DLL log to write\r\n");
        return;
    }
    DPRINT("Writing DLL log\r\n");
    wii->fwrite(dllLog, sizeof(DllLogEntry), NUM_DLLS, file);
    wii->fclose(file);
    logNeedsWrite = false;
}
#endif

void* dll_load_hook(int dll_id, u16 param) {
    //replaces the body of dll_load()
    //OSReport("dll_load(0x%04X, 0x%04X)\n", dll_id, param);
    #if LOG_DLLS
        dllLogUpdate(dll_id, param, __builtin_return_address(0));
    #endif

    //HACK to fix dragrockbot
    if(dll_id == 0x27F) dll_id = 0x131;

    if(dll_id >= 0 && dll_id < NUM_DLLS) {
        DLL *dll = g_dlls[dll_id];
        if(g_dllRefCount[dll_id] == 0) {
            if(dll->initialise == NULL) { /* do nothing */ }
            else if(!PTR_VALID(dll->initialise)) {
                OSReport("ERR: Trying to load DLL 0x%04X with invalid constructor 0x%08X\n",
                    dll_id, dll->initialise);
            }
            else dll->initialise(dll, param);
        }
        g_dllRefCount[dll_id]++;
        g_dllsLoaded[dll_id] = &dll->functions;
        return &g_dllsLoaded[dll_id];
    }
    else {
        OSReport("ERR: Trying to load invalid DLL 0x%04X\n", dll_id);
        return NULL;
    }
}

void dllHooksInit() {
    hookBranch(0x80013ec8, dll_load_hook, 0);
}

