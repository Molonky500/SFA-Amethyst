/** Bootstrap, runs at startup and installs other hooks.
 */
#include "main.h"
#include "revolution/os.h"
#include "revolution/pad.h"

//hack to ensure nothing is at offset 0 because that makes relocation difficult.
int fart __attribute__((section(".offsetzero"))) = 0x52656E61;

//GC regs. some will change if we're in Wii mode.
volatile u16 *_viReg  = (volatile u16*)0xCC002000;
volatile u32 *_piReg  = (volatile u32*)0xCC003000;
volatile u16 *_memReg = (volatile u16*)0xCC004000;
volatile u16 *_dspReg = (volatile u16*)0xCC005000;
volatile u32 *_ipcReg = (volatile u32*)0xCC000000; //these three
volatile u32 *_exiReg = (volatile u32*)0xCC006800; //will change
volatile u32 *_aiReg  = (volatile u32*)0xCC006C00; //for Wii

u64 tBootStart;
u32 debugCheats = 0; //DebugCheat
s16 overrideColorScale = -1;
u8 overrideFov = 60;
u8 furFxMode = 0; //FurFxMode
u16 dayOfYear, curYear;
u32 curTimeStamp;
bool bRumbleBlur = false;
bool bDisableParticleFx = false;
bool bNoAimSnap = false;
bool bSensitiveAim = false;

static inline void checkTime() {
    u64 ticks = __OSGetSystemTime();
    //this is necessary to make gcc not try to use soft float here.
    u32 tHi = ticks >> 32;
    u32 tLo = ticks & 0xFFFFFFFF;
    float fTicks = (float)tLo + (float)(tHi * 4294967296.0f);

    //note timestamp here is seconds since 2000-01-01
    //everything says this should be / 4 but I only get anything
    //sensible with / 2.
    float secs = fTicks / ((double)__OSBusClock / 2.0);
    curTimeStamp = (u32)secs;
    int days  = secs / 86400.0f; //non-leap days
    curYear = secs / 31556908.8f; //approximate average
    dayOfYear = days % 365 - (curYear / 4);
    //bool leap = (curYear % 4) == 0;
    //debugPrintf("Y %d D %d L %d\n", curYear, dayOfYear, leap);
}


static inline void doPadMainLoop() {
    static u32 prevBtn3 = 0, prevBtn4 = 0;
    u32 bHeld3 = controllerStates[2].button;
    u32 bHeld4 = controllerStates[3].button;
    u32 bPressed3 = bHeld3 & ~prevBtn3;
    u32 bPressed4 = bHeld4 & ~prevBtn4;
    prevBtn3 = bHeld3;
    prevBtn4 = bHeld4;

    //Pad 3 start: toggle time stop
    //if(bPressed3 & PAD_BUTTON_START) timeStop = !timeStop;
    if(bPressed3 & PAD_BUTTON_START) gPlayerMoveWhileTimeStopped = !gPlayerMoveWhileTimeStopped;

    //while time stopped, pad 3 Y to advance one tick
    static bool isStep = false;
    if(timeStop) {
        if(bPressed3 & PAD_BUTTON_Y) {
            isStep = true;
            timeStop = false;
        }
    }
    else if(isStep) {
        timeStop = true;
        isStep = false;
    }

    //Pad 3 Up: cycle heap displays
    if(bPressed3 & PAD_BUTTON_UP) {
        heapDrawMode++;
        if(heapDrawMode >= NUM_HEAP_DRAW_MODES) heapDrawMode = 0;
    }
    if(bPressed3 & PAD_BUTTON_DOWN) printHeaps();
    //if(bPressed3 & PAD_BUTTON_DOWN) mapLoadDataFiles(0xC);

    //Pad 4 Start: abort cutscene
    //XXX also stop dialogue
    if(bPressed4 & PAD_BUTTON_MENU) endObjSequence(curSeqNo);
}

static inline void sanityCheck() {
    if(furFxMode >= NUM_FURFX_MODES) furFxMode = 0;
    if(backpackMode >= NUM_BACKPACK_MODES) backpackMode = 0;
    if(overridePlayerNo >= NUM_PLAYER_IDS) overridePlayerNo = 0;
    if(overrideMinimapSize >= NUM_MINIMAP_SIZES) overrideMinimapSize = 0;
    if(overrideFov == 0) overrideFov = 60;
    if(timeDelta < 0.5 || timeDelta > 2.0) {
        OSReport("timeDelta = %f\n", timeDelta);
    }
    if(timeDelta < 0.1) timeDelta = 0.1;
}

static inline void doFovOverride() {
    if(overrideFov != 60 && !CameraParamsViewfinder) {
        //override if not in first person
        fovY = overrideFov;
        firstPersonFovY = overrideFov;
        if(pCamera) pCamera->fov = overrideFov;
        if(cameraMtxVar57) cameraMtxVar57->mtx1[3][0] = overrideFov;
    }
}

static inline void doFurFx() {
    switch(furFxMode) {
        case FURFX_NORMAL:
            WRITE16(0x800414E2, 0);
            WRITE32(0x800414B4, 0x9421FFE0); //original opcode
            ICInvalidateRange((void*)0x800414B4, 0x40);
            break;
        case FURFX_ALWAYS:
            WRITE16(0x800414E2, 0x1312); //arbitrary constant
            WRITE32(0x800414B4, 0x9421FFE0); //original opcode
            ICInvalidateRange((void*)0x800414B4, 0x40);
            break;
        default: //case FURFX_NEVER:
            WRITE_BLR(0x800414B4);
            ICInvalidateRange((void*)0x800414B4, 0x40);
    }
}

static inline void doAspectRatio() {
    //correct aspect ratio
    if(renderFlags & RenderFlag_Widescreen) {
        viewportAspect = 16.0 / 9.6;
        viewportAspectWidescreen = 16.0 / 9.6;
        viewportAspectWidescreenShadows = 16.0 / 9.6;
    }
    else {
        viewportAspect = 5.0 / 4.0;
        viewportAspectNotWidescreen = 5.0 / 4.0;
        //shadows use same address as main viewport for non-widescreen
    }
}

static inline void doCheats() {
    ObjInstance *arwing = getArwing();
    u8 *arwingState = arwing ? (u8*)arwing->state : NULL;
    PlayerCharState *playerState =
        &saveData.curSaveGame.charState[saveData.curSaveGame.character];
    if(debugCheats & DBGCHT_INF_HP) {
        playerState->curHealth = playerState->maxHealth;
        if(arwingState) {
            arwingState[0x468] = arwingState[0x469]; //curHealth = maxHealth
        }
    }
    if(debugCheats & DBGCHT_INF_MP) {
        playerState->curMagic = playerState->maxMagic;
    }
    if(debugCheats & DBGCHT_INF_MONEY) {
        playerState->money = 255;
    }
    if(debugCheats & DBGCHT_INF_LIVES) {
        playerState->curBafomDads = playerState->maxBafomDads;
    }
    if(debugCheats & DBGCHT_ENEMY_FROZEN) {
        //patch objIsFrozen() to always return true
        WRITE32(0x8002b048, 0x38600001);
    }
    else WRITE32(0x8002b048, 0x540307FE);
    ICInvalidateRange((void*)0x8002b048, 4);

    if(debugCheats & DBGCHT_INF_TRICKY) {
        saveData.curSaveGame.trickyEnergy = saveData.curSaveGame.maxTrickyEnergy;
    }

    if(arwingState) {
        WRITE32(0x8022D518, (debugCheats & DBGCHT_10_RINGS) ? 0x3860000A : 0x88630470);
        ICInvalidateRange((void*)0x8022D518, 4);
        if(debugCheats & DBGCHT_ARW_INF_BOMBS) {
            arwingState[0x44C] = 3; //nBombs = 3
        }
    }
}

static u16 titleTextFrameCount = 0;
void drawTitleText() {
    static const char *lines[] = {
        "Amethyst Edition v" MOD_VERSION_STR,
        "Built " __DATE__ " " __TIME__, NULL};

    //some TVs take a second to adjust to video mode change
    float alpha = (600 - titleTextFrameCount) / 300.0f;
    if(alpha <= 0.0f) return;
    if(alpha >= 1.0f) alpha = 1.0f;
    titleTextFrameCount++;

    Color4b color = {0xCC, 0xCC, 0xCC, (u8)(alpha * 255.0f)};
    int y=380;
    for(int i=0; lines[i]; i++) {
        //measure the line
        int width=0;
        drawText(lines[i], 0, 0, &width, NULL,
            TEXT_COLORED | TEXT_MEASURE,
            color, 1.0);
        //draw the line
        drawText(lines[i], (SCREEN_WIDTH / 2) - (width / 2),
            y, NULL, NULL, TEXT_COLORED,
            color, 1.0);
        y += 15;
    }
}

void mainLoopHook() {
    //replaces a bl to a do-nothing subroutine

    static bool firstRun = true;
    if(firstRun) {
        OSReport("main loop hook OK\n");
        firstRun = false;
    }

    sanityCheck();
    checkTime();
    doPadMainLoop();
    doFovOverride();
    doFurFx();

    WRITE32(0x800a4df4, bDisableParticleFx ? 0x4E800020 : 0x9421FED0);
    ICInvalidateRange((void*)0x800a4df4, 4);

    WRITE32(0x80148bc8, (debugTextFlags & DEBUGTEXT_TRICKY) ? 0x4BFEED80 : 0x9421FF90);
    ICInvalidateRange((void*)0x80148bc8, 4);

    WRITE32(0x8000E398, (cameraFlags & CAM_FLAG_NO_LETTERBOX) ? 0x38000000 : 0xA80D96A6);
    ICInvalidateRange((void*)0x8000E398, 4);

    WRITE32(0x8005ED10, (debugRenderFlags & DEBUGRENDER_DEBUG_MAP_GEOM) ? 0x60000000 : 0x408204B8);
    ICInvalidateRange((void*)0x8005ED10, 4);

    minimapMainLoopHook();
    mainLoopDebugPrint();
    runMenu();
    krystalMainLoop();
    doFreeMove();
    saveUpdateHook();
    gameBitHookUpdate();
    doAspectRatio();
    playerMainLoopHook();
    doCheats();

    //move camera while time is stopped in debug mode
    if(timeStop && (debugCameraMode != CAM_MODE_NORMAL) && !menuState) {
        cameraUpdate(1);
    }

    //XXX doesn't always work?
    if(overrideColorScale >= 0) colorScale = overrideColorScale;

    //HACK: correct WarpStone menu text box positions.
    //this is just for restoring the menu and isn't needed.
    //for(int i=0; i<6; i++) {
    //    WRITE16(0x802c79f6 + (i*0x20), (i+2) * 0x30);
    //}
    //WRITE16(0x803dd90c, 0x0100); //make menu visible

    doHudHacks();
    raceTimerUpdate();
    updateOsd();

    //static float test = 0;
    //debugPrintf("float test [%f] [%f]\n", test, -test);
    //test += 1.0f / 60.0f;

    if(curMapId != 0x3F) {
        //if not on title screen, reset this flag, so we can
        //load args again on next reset.
        didTryLoadArgs = 0;
        #if LOG_DLLS
            dllLogWrite();
        #endif
        #if LOG_OBJS
            objLogWrite();
        #endif
        //reset text so it will appear again next time
        titleTextFrameCount = 0;
    }
    else drawTitleText();
}

/* static u32 oldSaveGameInitialise = 0;
static void dll_SaveGame_initialise_hook(void *unused) {
    OSReport("SaveGame initialise");
    ((void (*)(void))oldSaveGameInitialise)();
} */

static inline void _initSaveHacks() {
    //if(IS_WII) return;
    hookBranch(0x800e7fb0, saveLoadHook, 1);
    hookBranch(0x80042ec4, saveMapLoadHook, 1);
    hookBranch(0x8007db50, saveShowMsgHook, 1);
    //disable "not same memory card you last saved with" check,
    //since save states trigger that.
    WRITE32(0x8007EF5C, 0x3B200000);
    WRITE32(0x8007F15C, 0x3B200000);

    //oldSaveGameInitialise = READ32(0x80311910);
    //WRITE32(0x80311910, dll_SaveGame_initialise_hook);
}

static inline void _initDebugPrintHacks() {
    hookBranch(0x80137948, debugPrintfHook, 0);
    WRITE32   (0x801378A8, 0x480000A0); //restore debugPrintf
    WRITE8    (0x80137317, 6); //smaller text for fixed-width mode
    WRITE16   (0x803E23B8, 0x3FA0); //smaller text
    WRITE32   (0x80137CF4, 0x3BFF000C); //smaller text for debugPrintfxy
    WRITE16   (0x801372AA, 8); //less line spacing
    if(consoleType & 0xF0000000) { //emulator
        //move debug print to edge of screen
        WRITE16(0x8013761A, 0); //min X at 320 screen width
        WRITE16(0x8013762E, 0); //min X at 640 screen width
        WRITE16(0x8013764A, 0); //min Y at 240 screen height
        WRITE16(0x8013765E, 0); //min Y at 480 screen height - causes a glitch at bottom when fading (or not?)
        //WRITE32(0x80137830, 0x38000000); //these two prevent fade glitch
        //WRITE32(0x80137688, 0x38000000);
    }
}

static inline void _initPauseMenuHacks() {
    WRITE32(0x8012A97C, 0x4BFBDD55); //disable save confirmation
    //disable some voices
    WRITE_NOP(0x8012A904);
    WRITE_NOP(0x8012A8C4);
    WRITE_NOP(0x8012A95C);
    WRITE_NOP(0x8012BA94);
    WRITE_NOP(0x8012B8DC);
    WRITE_NOP(0x8012B88C);
    WRITE_NOP(0x8012B8B4);
    WRITE_NOP(0x8012BD78);
}

static inline void _initCameraHacks() {
    //hookBranch(0x8010328c,        cameraUpdateHook,         1);
    hookBranch(0x801032b8,        cameraUpdateHook,         1);
    //hookBranch(0x80101e98,        cameraUpdateHook,         1);
    hookBranch((u32)padGetCX,     padGetCxHook,             0);
    hookBranch((u32)padGetCY,     padGetCyHook,             0);
    hookBranch((u32)padGetStickX, padGetStickXHook,         0);
    hookBranch((u32)padGetStickY, padGetStickYHook,         0);
    hookBranch(0x80133af0,        minimapButtonHeldHook,    1);
    hookBranch(0x80133afc,        minimapButtonPressedHook, 1);
    hookBranch(0x80108758,        viewFinderZoomHook,       1);

    //viWidth fix
    //XXX this makes things look all stretched in widescreen.
    //WRITE16(0x80049512, 704);
    //WRITE16(0x80049526, -32);
}

static inline void _initControllerHacks() {
    //enable all four controllers, which enables at least one debug function
    //(plus several more we added)
    WRITE8(0x80014B87, 4);
    WRITE8(0x80014BC7, 4);
    WRITE8(0x80014C1B, 4);
    WRITE8(0x80014C6F, 4);
    WRITE8(0x80014CC3, 4);
    WRITE8(0x80014D9F, 4);
    WRITE8(0x80014DDB, 4);
    WRITE8(0x80014E17, 4);
    WRITE8(0x80014E73, 4);
    WRITE8(0x80014EC7, 4);
    WRITE8(0x80014EEB, 4);
}

/* void pdaHook() {
    openMainMenu();
} */

void _pdaHook(void);
__asm__(
    "_pdaHook:                \n"
    ASM_FUNC_START(0x80)
    //"bl    pdaHook            \n"
    "bl    openMainMenu       \n"
    ASM_FUNC_END(0x80)
    "lis    0,  0x8013        \n" //skip actual on/off code
    "ori    0,  0,  0x3A94    \n"
    "mtctr  0                 \n"
    "bctr                     \n"
);

void _start(void) {
    tBootStart = __OSGetSystemTime();
    DPRINT("Patch running!");

    if(IS_WII) { //these regs have different addrs.
        //the loader patches the accesses in the original
        //game code but we need to do this part ourselves.
        _ipcReg = (u32*)0xCD000000;
        _exiReg = (u32*)0xCD006800;
        _aiReg  = (u32*)0xCD006C00;
    }

    //make staff swipe effect use less memory. XXX investigate how much is actually needed.
    //seems like it wants enough for 3000 swipes which is insane...
    //but with this amount we get some glitching. maybe it really does need this!?
    //but this is 60K x 3 = 180K, no way...
    //WRITE32(0x8016ef4c, 0x38602710);

    //Install hooks
    if(!IS_WII) hookBranch(0x80137df8, bsodHook, 1);
    //XXX integrate nicely
    initBootHacks();
    initBugFixes();
    dllHooksInit();
    objHooksinit();
    if(!runLoadingScreens_replaced) {
        runLoadingScreens_replaced = (void(*)())hookBranch(0x80020f2c,
            runLoadingScreens_hook, 1);
    }
    if(!startMsg_initDoneHook_replaced) {
        startMsg_initDoneHook_replaced = (void(*)())hookBranch(0x80021250,
            startMsg_initDoneHook, 1);
    }
    hookBranch(0x80020D4C, mainLoopHook, 1);
    hookBranch(0x8005c45c, motionBlurHook, 1);
    hookBranch(0x800d9e2c, hudDrawHook, 1);
    allocInit();
    perfMonInit();
    rngHooksInit();
    hookBranch(0x80133A54, _pdaHook, 0);
    hookBranch(0x8020d31c, worldMapHook, 1);

    krystalInit();
    _initSaveHacks();
    _initCameraHacks();
    _initPlayerHacks();
    gameBitHookInit();
    debugObjsInit();
    titleHooksInit();
    logHitsInit();
    renderHooksInit();
    initMapHacks();
    staffFxInit();
    _initDebugPrintHacks();
    _initPauseMenuHacks();
    envHooksInit();
    seqHooksInit();
    textHookInit();
    tweaks_init();
    initOsd();
    OSReport("updateOsd=%08x\r\n", updateOsd);

    if(IS_WII) {
        OSReport("Booting in Wii mode\n");
        wiiHooksInit();
    }
    else { //do the DVD init we replaced.
        OSReport("Booting in GC mode\n");
        _DVDInit();
    }

    DPRINT("Hooks installed!");
}

const char *languageNames[NUM_LANGUAGES] = {
    "English", //English
    "français", //French
    "Deutsch", //German
    "Italiano", //Italian
    "日本語", //Japanese
    "Español" //Spanish
};
const char *languageNamesShort[NUM_LANGUAGES] = {
    "EN", //English
    "FR", //French
    "DE", //German
    "IT", //Italian
    "JP", //Japanese
    "ES" //Spanish
};
void setGameLanguage(GameLanguageEnum lang) {
    curLanguage = lang;
    GameTextDir32 dir = curGameTextDir;
    gameTextLoadDir(GAMETEXT_DIR_Link); //load HUD texts
    gameTextLoadDir(dir); //then load current map's texts
}
