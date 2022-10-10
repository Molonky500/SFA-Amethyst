#ifndef __GAME_WII_IFACE_H__
#define __GAME_WII_IFACE_H__

//defines struct used by both the Wii-side loader
//and the GC-side mod code.
#define WII_IFACE_MAGIC 0xF0C5BABE

#define WM_FLAG_PRESENT 0x01 //Wiimote is present
#define WM_FLAG_WORKING 0x02 //Wiimote is connected and working
#define WM_FLAG_MPLUS   0x04 //MotionPlus is connected
//following can be set by GC code
#define WM_FLAG_RUMBLE  0x08 //Motor is on
#define WM_FLAG_LED1    0x10 //LED 1 is on
#define WM_FLAG_LED2    0x20 //LED 2 is on
#define WM_FLAG_LED3    0x40 //LED 4 is on
#define WM_FLAG_LED4    0x80 //LED 8 is on
#define WM_FLAG_LED_MASK 0xF0
#define WM_FLAG_GC_CONTROLLED_MASK (WM_FLAG_RUMBLE | WM_FLAG_LED_MASK)
#define GAME_MAX_WIIMOTES 4

typedef struct {
    //readable - the Wiimote itself
    u8 flags; //WM_FLAG_*
    u8 battery; //0-255
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
} GameWiiInterface;

//we store the pointer here in a place that's normally
//zero for Wii games, and would be part of __start
//for this game
#define WII_IFACE_PTR (*(GameWiiInterface**)0x80003194)

#endif //__GAME_WII_IFACE_H__
