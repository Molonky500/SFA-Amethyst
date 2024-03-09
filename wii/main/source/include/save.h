#ifndef _SAVE_H_
#define _SAVE_H_

#define CARD_RESULT_UNLOCKED        1
#define CARD_RESULT_READY           0
#define CARD_RESULT_BUSY            -1
#define CARD_RESULT_WRONGDEVICE     -2
#define CARD_RESULT_NOCARD          -3
#define CARD_RESULT_NOFILE          -4
#define CARD_RESULT_IOERROR         -5
#define CARD_RESULT_BROKEN          -6
#define CARD_RESULT_EXIST           -7
#define CARD_RESULT_NOENT           -8
#define CARD_RESULT_INSSPACE        -9
#define CARD_RESULT_NOPERM          -10
#define CARD_RESULT_LIMIT           -11
#define CARD_RESULT_NAMETOOLONG     -12
#define CARD_RESULT_ENCODING        -13
#define CARD_RESULT_CANCELED        -14
#define CARD_RESULT_FATAL_ERROR     -128

typedef void (*CARDCallback)(s32 chan, s32 result);

#define MAX_OBJECT_SAVES 63 //not 64
#define MAX_TIME_SAVES 256

typedef struct {
    u8  exists;
    u8  unused01; //Amethyst: extra options
    u8  bSubtitlesOn;
    u8  unusedHudSetting; //Amethyst: camera options; copied to a never-read variable
    u8  unusedCameraSetting; //copied to bitmask 0x18 of camera->flags_0x141
    u8  unused05;
    u8  bWidescreen;
    u8  unused07; //Amethyst: current character
    u8  bRumbleEnabled;
    u8  soundMode;
    u8  musicVolume;
    u8  sfxVolume;
    u8  cutsceneVolume;
    u8  unused0D; //Amethyst: options
    u16 unused0E; //Amethyst: FOV, map opacity
    u32 unlockedCheats;
    u32 activeCheats;
    u32 unk18; //probably unused
} SaveGameSettings;

typedef struct {
    u32 score; //times 2, plus 1 if 10 gold rings
    char name[4];
} SavedHighScore;

typedef struct {
   SaveGameSettings settings;
   SavedHighScore scoresToPlanet[5];
   SavedHighScore scoresDarkIce[5];
   SavedHighScore scoresCloudRunner[5];
   SavedHighScore scoresWallCity[5];
   SavedHighScore scoresDragRock[5];
} SaveSettingsAndScores;

typedef struct {
    u8 curHealth; //in 1/4 hearts
    u8 maxHealth;
    u8 field_02;
    u8 field_03;
    u16 curMagic;
    u16 maxMagic;
    u8 money;
    u8 curBafomDads;
    u8 maxBafomDads;
    u8 field_0B;
} PlayerCharState;

typedef struct {
    u8 energy;
    u8 maxEnergy;
    u8 playCount;
    u8 unk03; //probably unused
} SavedTrickyState;

typedef struct {
    s32 id;
    vec3f pos;
} SaveGameObjectPos;

typedef struct PACKED {
    u32 id; //ObjUniqueId
    float time;
} SaveGameTimeLog;

typedef struct {
    vec3f pos;
    s8 rotX; //high byte of vec3s.x
    s8 mapLayer;
    s8 mapNo;
    s8 unk; //probably unused
} PlayerCharPos;

typedef struct {
    float timeOfDay;
    short envFxActIdx[5];
    short envFxActIdx2[3];
    vec3i skyObjPos[3];
    u8 unk38;
    u8 unk39;
    u8 unk3A;
    u8 unk3B;
    u8 unk3C;
    u8 unk3D;
    u8 unk3E;
    u8 unk3F;
    u8 flags40; //1:enable clouds+lights; 8:enable lights; 20:heat FX
    s8 skyObjIdx[3];
} SaveGameEnvState;

typedef struct {
    PlayerCharState charState[2];
    SavedTrickyState tricky;
    char saveFileName[4]; //null terminated
    s8 character;
    u8 flags21; //80=erase me; 60=save slot
    u8 flag_0x22;
    u8 unk23;
    u8 gameBits2[324];
    SaveGameObjectPos objs[63];
    u8 texts[5]; //hint texts
    u8 completion; //percent = (this / 187) * 100
    u8 numTexts;
    float playTime; //frame count
    u8 gameBits1[116];
    u8 gameBits3[172];
    PlayerCharPos charPos[2];
    s16 camVar6A4; //related to camera actions?
    SaveGameEnvState env; //environment settings
} SaveGame;

typedef struct {
    SaveSettingsAndScores global;
    SaveGame save; //just one slot
} RamSaveData;

#endif //_SAVE_H_
