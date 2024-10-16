#include "main.h"
#include "revolution/os.h"

//XXX figure out some better way, maybe using the original GameText system

//assign a table to each game language
//untranslated will use English until translation happens
const char **stringTables[] = {
    strings_English, //English
    strings_English, //French
    strings_English, //German
    strings_English, //Italian
    strings_Japanese, //Japanese
    strings_English, //Spanish
};

const char* T(const char *s) {
    const char **table = stringTables[curLanguage];
    if(table == strings_English) return s;

    //look up the string in the English table (XXX cache?)
    for(int i=0; strings_English[i]; i++) {
        //string interning for constant strings
        if(strings_English[i] == s) return table[i];
    }

    //could loop again and strcmp here...
    OSReport("String not found in translation table: \"%s\"", s);
    return s;
}

//debug strings which should not be translated
const char *objTypeNames[] = { //NULL for unknown
    "Player", //0,
    "Tricky", //1,
    "Fireball", //2, //light? also DrakorMissile
    "NPC", //3, //Baddie, BabyCloudRunner
    "Collectible", //4, //also WM_Column, MMP_moonrock
    "Pushable", //5,
    "Parent", //6, //obj that has children
    "StaticCamera", //7,
    "PlayerObj", //8,
    "LevelControl", //9, //and Shop
    "Rideable", //10, //Bike, CloudRunner
    "ECSH_Shrine", //11, //Test of Skill
    NULL, //12
    NULL, //13
    "Door", //14,
    "LockOrSeqObj", //15,
    "Carryable", //16,
    NULL, //17
    NULL, //18
    "DIMDIsmountPoint", //19, //also DR_CloudPerch
    "WaterCurrent", //20,
    NULL, //21
    "Unk16", //22, //Spellstone?, bomb barrel, conveyor
    "DFropenode", //23,
    "DR_CageWithCloudRunner", //24,
    "GunPowderBarrel", //25,
    "ExplodeAnimator", //26,
    "WaveAnimator", //27,
    "LFXEmitter", //28,
    NULL, //29
    "TrickyGuardSpot", //30, //also Attractor, DBHoleControl1, SpellStone; baddie related?
    NULL, //31
    "BabyCloudRunner", //32,
    "Explodable", //33,
    NULL, //34
    "WallAnimator", //35,
    "DBEgg", //36,
    "Player25", //37,
    "Rideable26", //38, //Hightop, Arwing, DR_CloudRunner
    NULL, //39
    NULL, //40
    NULL, //41
    NULL, //42
    NULL, //43
    NULL, //44
    NULL, //45
    "MoonSeedPlantingSpot", //46,
    "RollingBarrel", //47,
    "Firefly", //48,
    "TrickyInteractable", //49, //things Tricky can activate
    "TrickyAutoHeel", //50,
    NULL, //51
    "MagicPlant", //52,
    "PointLight", //53,
    NULL, //54
    "DR_Shackle", //55,
    NULL, //56
    "DR_CloudPerch", //57,
    "BarrelGenerator", //58,
    NULL, //59
    "NW_ice", //60,
    "NW_animice", //61,
    "DLL_F7", //62, //also magicplant
    "CCgasvent", //63,
    "siderepel", //64,
    "StaffActivated", //65,
    NULL, //66
    NULL, //67
    NULL, //68
    "BossDrakor", //69,
    "DrakorHoverPad", //70,
    "Food", //71,
    "Lightning", //72,
    "WindLift", //73,
    "FirePipe", //74,
    "TrickyWarp", //75,
    "Timer", //76,
    "Speakable", //77, //NPC you can talk to
    "SpiritDoorSpirit", //78,
    "FuelCell", //79,
    "Whirlpool", //80,
    "XYZAnimator", //81,
    "ArwingBomb", //82,
    "PressureSwitch", //83,
};
