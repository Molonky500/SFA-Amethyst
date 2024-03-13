#ifndef __GAME_WII_IFACE_H__
#define __GAME_WII_IFACE_H__

//defines struct used by both the Wii-side loader
//and the GC-side mod code.
#define WII_IFACE_MAGIC 0xF0C5BABE

#define WM_FLAG_PRESENT   0x0001 //Wiimote is present
#define WM_FLAG_WORKING   0x0002 //Wiimote is connected and working
#define WM_FLAG_MPLUS     0x0004 //MotionPlus is connected
//following can be set by GC code
#define WM_FLAG_RUMBLE    0x0008 //Motor is on
#define WM_FLAG_LED1      0x0010 //LED 1 is on
#define WM_FLAG_LED2      0x0020 //LED 2 is on
#define WM_FLAG_LED3      0x0040 //LED 4 is on
#define WM_FLAG_LED4      0x0080 //LED 8 is on
#define WM_FLAG_LED_MASK  0x00F0
#define WM_FLAG_IR_VALID  0x0100
#define WM_FLAG_IR_SMOOTH 0x0200
#define WM_FLAG_GC_CONTROLLED_MASK (WM_FLAG_RUMBLE | WM_FLAG_LED_MASK)
#define GAME_MAX_WIIMOTES 4

typedef struct {
    //readable - the Wiimote itself
    u16 flags; //WM_FLAG_*
    u8  battery; //0-255
    u8  irNumDots;
    u32 btnsDown, btnsHeld, btnsUp;
    float accel[3], orient[3], gforce[3];
    float ir[3], irAngle;

    //MotionPlus
    s16 rotation[3];

    s8 expType; //which expansion device type
    union { //the expansion device
        struct { //Nunchuk
            s8 joystick[2];
            u32 btnsDown, btnsHeld, btnsUp;
            float accel[3], orient[3], gforce[3];
        } nunchuk;

        struct { //Classic Controller
            s8 lStick[2], rStick[2];
            u32 btnsDown, btnsHeld, btnsUp;
            u8 lTrig, rTrig; //0-255
        } classic;
    } exp;
} GameWiimoteState;

typedef struct {
    u32 magic;
    GameWiimoteState wiimote[GAME_MAX_WIIMOTES];

    //function pointers filled in by loader for game to use.
    void (*updateWiimotes)(void);
    FILE* (*fopen)(const char *filename, const char *mode);
    int (*fclose)(FILE *stream);
    size_t (*fread)(void *buffer, size_t size, size_t count, FILE *stream);
    size_t (*fwrite)(const void *buffer, size_t size, size_t count, FILE *stream);
    DIR* (*opendir)(const char *dirname);
    int (*closedir)(DIR *dirp);
    struct dirent* (*readdir)(DIR *dirp);
    void (*rewinddir)(DIR *dirp);
    void (*seekdir)(DIR *dirp, long loc);
    long (*telldir)(DIR *dirp);
    int (*stat)(const char *path, struct stat *buf);

    //filled in by game for Wii code to use.
    //these are functions that are part of the mod code and thus don't load
    //at a fixed address.
    int (*drawText)(const char *str, int x, int y, int *outX, int *outY, u32 flags,
        u32 color, float scale);
    void (*addOsdMessage)(const char *str, int frames);
    void (*drawBox)(float x, float y, int w, int h, u8 opacity, bool fill);
    int (*showPopupMessage)(const char *text, const char **options);

    const char *gameRootDir;
    u8 language; //read from Wii config
} GameWiiInterface;

//we store the pointer here in a place that's normally
//zero for Wii games, and would be part of __start
//for this game
#define WII_IFACE_PTR (*(GameWiiInterface**)0x80003194)

#endif //__GAME_WII_IFACE_H__
