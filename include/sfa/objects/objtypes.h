//not to be confused with ObjCatId
typedef enum { //type:u32
    ObjType_Player = 0,
    ObjType_Tricky = 1,
    ObjType_Fireball = 2, //light? also DrakorMissile
    ObjType_NPC = 3, //Baddie, BabyCloudRunner
    ObjType_Collectible = 4, //also WM_Column, MMP_moonrock
    ObjType_Pushable = 5,
    ObjType_Parent = 6, //obj that has children
    ObjType_StaticCamera = 7,
    ObjType_PlayerObj = 8,
    ObjType_LevelControl = 9, //and Shop
    ObjType_Rideable = 10, //Bike, CloudRunner
    ObjType_ECSH_Shrine = 11, //Test of Skill

    ObjType_Door = 14,
    ObjType_LockOrSeqObj = 15,
    ObjType_Carryable = 16,

    ObjType_DIMDIsmountPoint = 19, //also DR_CloudPerch
    ObjType_WaterCurrent = 20,

    ObjType_Unk16 = 22, //Spellstone?, bomb barrel, conveyor
    ObjType_DFropenode = 23,
    ObjType_DR_CageWithCloudRunner = 24,
    ObjType_GunPowderBarrel = 25,
    ObjType_ExplodeAnimator = 26,
    ObjType_WaveAnimator = 27,
    ObjType_LFXEmitter = 28,

    ObjType_TrickyGuardSpot = 30, //also Attractor, DBHoleControl1, SpellStone; baddie related?

    ObjType_BabyCloudRunner = 32,
    ObjType_Explodable = 33,

    ObjType_WallAnimator = 35,
    ObjType_DBEgg = 36,
    ObjType_Player25 = 37,
    ObjType_Rideable26 = 38, //Hightop, Arwing, DR_CloudRunner

    ObjType_MoonSeedPlantingSpot = 46,
    ObjType_RollingBarrel = 47,
    ObjType_Firefly = 48,
    ObjType_TrickyInteractable = 49, //things Tricky can activate
    ObjType_TrickyAutoHeel = 50,

    ObjType_MagicPlant = 52,
    ObjType_PointLight = 53,

    ObjType_DR_Shackle = 55,

    ObjType_DR_CloudPerch = 57,
    ObjType_BarrelGenerator = 58,

    ObjType_NW_ice = 60,
    ObjType_NW_animice = 61,
    ObjType_DLL_F7 = 62, //also magicplant
    ObjType_CCgasvent = 63,
    ObjType_siderepel = 64,
    ObjType_StaffActivated = 65,

    ObjType_BossDrakor = 69,
    ObjType_DrakorHoverPad = 70,
    ObjType_Food = 71,
    ObjType_Lightning = 72,
    ObjType_WindLift = 73,
    ObjType_FirePipe = 74,
    ObjType_TrickyWarp = 75,
    ObjType_Timer = 76,
    ObjType_Speakable = 77, //NPC you can talk to
    ObjType_SpiritDoorSpirit = 78,
    ObjType_FuelCell = 79,
    ObjType_Whirlpool = 80,
    ObjType_XYZAnimator = 81,
    ObjType_ArwingBomb = 82,
    ObjType_PressureSwitch = 83,
    MAX_OBJTYPES = 84
} ObjTypeId;

extern const char *objTypeNames[]; //strings.c
