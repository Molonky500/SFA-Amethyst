#ifndef __GAMECONTROLS_H__
#define __GAMECONTROLS_H__

typedef struct {
    u16 buttons;
    s8 stickX, stickY;
    s8 cX, cY;
    u8 lTrig, rTrig;
    u16 unk08;
    s8 state; //-1 = not present
    u8 unk0B; //padding?
} GameControllerState;

//two pairs of four, alternating
#define controllerState ((GameControllerState*)0x803398f0)
#define whichControllerFrame (*(u8*)0x803dc94c)

#endif //__GAMECONTROLS_H__
