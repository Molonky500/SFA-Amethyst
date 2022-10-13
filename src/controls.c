#include "main.h"

void displayControllerState(int iPad, PADStatus *pad) {
    //display input for debug
    char msg[64];
    char *m = msg;
    *(m++) = (pad->button & PAD_BUTTON_A)     ? 'A' : '.';
    *(m++) = (pad->button & PAD_BUTTON_B)     ? 'B' : '.';
    *(m++) = (pad->button & PAD_BUTTON_X)     ? 'X' : '.';
    *(m++) = (pad->button & PAD_BUTTON_Y)     ? 'Y' : '.';
    *(m++) = (pad->button & PAD_BUTTON_START) ? 'S' : '.';
    *(m++) = (pad->button & PAD_TRIGGER_L)    ? 'L' : '.';
    *(m++) = (pad->button & PAD_TRIGGER_R)    ? 'R' : '.';
    *(m++) = (pad->button & PAD_TRIGGER_Z)    ? 'Z' : '.';
    *(m++) = (pad->button & PAD_BUTTON_UP)    ? '^' : '.';
    *(m++) = (pad->button & PAD_BUTTON_RIGHT) ? '>' : '.';
    *(m++) = (pad->button & PAD_BUTTON_DOWN)  ? 'V' : '.';
    *(m++) = (pad->button & PAD_BUTTON_LEFT)  ? '<' : '.';
    *(m++) = 0;

    debugPrintf("%s J %3d,%3d C %3d,%3d", msg,
        pad->stickX, pad->stickY, pad->substickX, pad->substickY);
    //split up because of sprintf bug
    debugPrintf("L%3d R%3d\n", pad->triggerLeft, pad->triggerRight);
}

void displayWiimoteState(GameWiimoteState *wp) {
    debugPrintf(DPRINT_FIXED " B%3d%% IR%d, %d, %dcm ang%d ",
        (int)(((float)wp->battery / 255.0f) * 100.0f),
        (int)wp->ir[0], (int)wp->ir[1], (int)(wp->ir[2] * 100.0f),
        (int)(wp->irAngle));
    debugPrintf("A %3d,%3d,%3d ",
        (int)wp->accel[0], (int)wp->accel[1], (int)wp->accel[2]);
    debugPrintf("O %3d,%3d,%3d ",
        (int)wp->orient[0], (int)wp->orient[1], (int)wp->orient[2]);
    debugPrintf("G %3d,%3d,%3d = %d ",
        (int)wp->gforce[0], (int)wp->gforce[1], (int)wp->gforce[2],
        (int)(ABS(wp->gforce[0])+
            ABS(wp->gforce[1])+
            ABS(wp->gforce[2]))
        );

    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            debugPrintf("\nNC: J %3d,%3d A %3d,%3d,%3d ",
                wp->exp.nunchuk.joystick[0],
                wp->exp.nunchuk.joystick[1],
                (int)wp->exp.nunchuk.accel[0],
                (int)wp->exp.nunchuk.accel[1],
                (int)wp->exp.nunchuk.accel[2]);
            debugPrintf("O %3d,%3d,%3d ",
                (int)wp->exp.nunchuk.orient[0],
                (int)wp->exp.nunchuk.orient[1],
                (int)wp->exp.nunchuk.orient[2]);
            debugPrintf("G %3d,%3d,%3d = %d\n",
                (int)(wp->exp.nunchuk.gforce[0] * 100.0f),
                (int)(wp->exp.nunchuk.gforce[1] * 100.0f),
                (int)(wp->exp.nunchuk.gforce[2] * 100.0f),
                (int)(ABS(wp->exp.nunchuk.gforce[0])+
                    ABS(wp->exp.nunchuk.gforce[1])+
                    ABS(wp->exp.nunchuk.gforce[2]))
                );
            break;
        }
        case WPAD_EXP_CLASSIC: {
            debugPrintf("JL %3d,%3d JR %3d,%3d L%3d R%3d\n",
                wp->exp.classic.lStick[0],
                wp->exp.classic.lStick[1],
                wp->exp.classic.rStick[0],
                wp->exp.classic.rStick[1],
                wp->exp.classic.lTrig,
                wp->exp.classic.rTrig);
            break;
        }
        default: break;
    }
    debugPrintf("\n" DPRINT_NOFIXED);
}

void adjustPlayerRotation(s16 x, s16 y, PADStatus *pad) {
    pPlayer->pos.rotation.x -= x;
    pPlayer->pos.rotation.y -= y;
    *(s16*)(pPlayer->state + 0x01A) -= x;
    *(s16*)(pPlayer->state + 0x478) -= x;
    *(s16*)(pPlayer->state + 0x484) -= x;
    *(s16*)(pPlayer->state + 0x492) -= x;
    *(s16*)(pPlayer->state + 0x494) -= x;
    *(s32*)(pPlayer->state + 0x5C0) -= x;
    ObjInstance *ride = *(ObjInstance**)(pPlayer->state + 0x7F0);
    if(ride) {
        void *rState = ride->state;
        ride->pos.rotation.x -= x;
        ride->pos.rotation.y -= y;
        if(ride->catId == 0x2E) { //bike
            *(s16*)(rState+0x40C) -= x;
            *(s16*)(rState+0x40E) -= x;
            *(s16*)(rState+0x41C) -= y;
        }
        else if(ride->catId == 0x30) { //cloudrunner
            *(s16*)(rState+0x2C) -= x / 2;
            *(s16*)(rState+0x2E) -= y / 2;
            *(s32*)(rState+0x70) -= x / 2;
            *(s32*)(rState+0x74) -= y / 2;
            //pad->stickX += x;
            //pad->stickY += y;
            //XXX IR aiming
        }
    }
}

u32 mapNunchukButtons(GameWiimoteState *wp, u32 btns) {
    u32 result = 0;
    if(btns & WPAD_NUNCHUK_BUTTON_Z) result |= PAD_TRIGGER_L;
    if(btns & WPAD_NUNCHUK_BUTTON_C) result |= PAD_BUTTON_Y;
    return result;
}

u32 mapClassicButtons(GameWiimoteState *wp, u32 btns) {
    u32 result = 0;
    //Nintendo pls what is consistency
    if(btns & WPAD_CLASSIC_BUTTON_A)     result |= PAD_BUTTON_X;
    if(btns & WPAD_CLASSIC_BUTTON_B)     result |= PAD_BUTTON_A;
    if(btns & WPAD_CLASSIC_BUTTON_X)     result |= PAD_BUTTON_Y;
    if(btns & WPAD_CLASSIC_BUTTON_Y)     result |= PAD_BUTTON_B;
    if(btns & WPAD_CLASSIC_BUTTON_ZR)    result |= PAD_TRIGGER_Z;
    if(btns & WPAD_CLASSIC_BUTTON_PLUS)  result |= PAD_BUTTON_START;
    if(btns & WPAD_CLASSIC_BUTTON_HOME)  result |= PAD_BUTTON_START;
    if(btns & WPAD_CLASSIC_BUTTON_UP)    result |= PAD_BUTTON_UP;
    if(btns & WPAD_CLASSIC_BUTTON_DOWN)  result |= PAD_BUTTON_DOWN;
    if(btns & WPAD_CLASSIC_BUTTON_LEFT)  result |= PAD_BUTTON_LEFT;
    if(btns & WPAD_CLASSIC_BUTTON_RIGHT) result |= PAD_BUTTON_RIGHT;
    return result;
}

u32 mapWiimoteButtons(GameWiimoteState *wp, u32 btns) {
    //generate GC button mask corresponding to Wii button
    //state on given pad.
    //mapping chosen arbitrarily to fit the game.
    u32 result = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            result |= mapNunchukButtons(wp, btns);
            break;
        }
        case WPAD_EXP_CLASSIC: {
            result |= mapClassicButtons(wp, btns);
            return result; //do not use the regular Wiimote buttons
        }
        default: break;
    }
    if(btns & WPAD_BUTTON_A)     result |= PAD_BUTTON_A;
    if(btns & WPAD_BUTTON_B)     result |= PAD_BUTTON_B;
    if(btns & WPAD_BUTTON_1)     result |= PAD_BUTTON_X;
    if(btns & WPAD_BUTTON_2)     result |= PAD_TRIGGER_R;
    if(btns & WPAD_BUTTON_MINUS) result |= PAD_TRIGGER_Z;
    if(btns & WPAD_BUTTON_PLUS)  result |= PAD_BUTTON_START;
    if(btns & WPAD_BUTTON_HOME)  result |= PAD_BUTTON_START;
    if(btns & WPAD_BUTTON_UP)    result |= PAD_BUTTON_UP;
    if(btns & WPAD_BUTTON_DOWN)  result |= PAD_BUTTON_DOWN;
    if(btns & WPAD_BUTTON_LEFT)  result |= PAD_BUTTON_LEFT;
    if(btns & WPAD_BUTTON_RIGHT) result |= PAD_BUTTON_RIGHT;
    return result;
}

static float prevAimX=0, prevAimY=0;
bool applyAimToStaff(GameWiimoteState *wp, PADStatus *pad) {
    if(!pPlayer) return false;
    //don't apply to title screen Fox, Arwing, etc
    if(pPlayer->catId != ObjCatId_Player) return false;
    void *state = pPlayer->state;
    if(!state) return false;

    float x = wp->ir[0];
    float y = wp->ir[1];
    x = (x + prevAimX) / 2.0f; //smoothing
    y = (y + prevAimY) / 2.0f;
    prevAimX = x;
    prevAimY = y;

    s16 stateNo = *(s16*)(state+0x274);
    switch(stateNo) {
        case 0x2C: { //aiming
            //set the staff aim coordinates
            *(float*)(state+0x788) = x;
            *(float*)(state+0x78C) = y;
            //adjust the physical staff motion
            *(float*)(state+0x7B8) = MIN(1.0f, MAX(-1.0f, (x/640.0f)));
            *(float*)(state+0x7BC) = MIN(1.0f, MAX(-1.0f, (y/480.0f)));
            return true; //we are in an aiming state
        }
        default: {
            //adjust the physical staff motion
            *(float*)(state+0x7B8) = MIN(1.0f, MAX(-1.0f, (x/640.0f)));
            *(float*)(state+0x7BC) = MIN(1.0f, MAX(-1.0f, (y/480.0f)));
            return false;
        }
    }
}

void applyJoystickInputs(GameWiimoteState *wp, PADStatus *pad) {
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            pad->stickX = wp->exp.nunchuk.joystick[0];
            pad->stickY = wp->exp.nunchuk.joystick[1];
            break;
        }
        case WPAD_EXP_CLASSIC: {
            pad->stickX       = wp->exp.classic.lStick[0];
            pad->stickY       = wp->exp.classic.lStick[1];
            pad->substickX    = wp->exp.classic.rStick[0];
            pad->substickY    = wp->exp.classic.rStick[1];
            pad->triggerLeft  = wp->exp.classic.lTrig;
            pad->triggerRight = wp->exp.classic.rTrig;
            break;
        }
        default: break;
    }
}

void doSwingGestures(GameWiimoteState *wp, PADStatus *pad, u32 *bDown) {
    s16 stateNo = *(s16*)(pPlayer->state+0x274);

    //swing to roll
    if(wp->expType == WPAD_EXP_NUNCHUK && (
    ABS(wp->exp.nunchuk.gforce[0]) +
    ABS(wp->exp.nunchuk.gforce[1]) +
    ABS(wp->exp.nunchuk.gforce[2]) >= 1.1f)) {
        OSReport("rollin' rollin' rollin'\n");
        *bDown |= PAD_BUTTON_X;
    }

    //swing to attack
    switch(stateNo) {
        case 0x01: //idle
        case 0x02: { //moving
            if(*(u8*)(pPlayer->state+0x8B3) != 0) { //staff in hand
                if(ABS(wp->gforce[0]) +
                ABS(wp->gforce[1]) +
                ABS(wp->gforce[2]) >= 8.0f) {
                    OSReport("swing it!\n");
                    *bDown |= PAD_BUTTON_A;
                }
            }
            break;
        }
        case 0x18: { //riding a bike/CloudRunner
            if(wp->expType == WPAD_EXP_NUNCHUK) {
                //XXX is the orientation sensor broken
                //or is it normal that X is always 0?
                adjustPlayerRotation(
                    wp->exp.nunchuk.orient[2] * 12,
                    wp->exp.nunchuk.orient[1] * 10, pad);
                //maybe forward tilt can be accel/brake instead
            }
            break;
        }
        case 0x1A: { //riding CloudRunner (DragRock?)
            if(wp->expType == WPAD_EXP_NUNCHUK) {
                pad->stickX += wp->exp.nunchuk.orient[2];
                pad->stickY += wp->exp.nunchuk.orient[1];
            }
            break;
        }
        default: break;
    }
}

void doAimControls(PADStatus *pad) {
    //try to let player walk around in this state.
    //this doesn't work...

    //clear flags that don't allow player to move
    /*(*(u32*)(pPlayer->state + 0)) = 0;
    (*(u8*) (pPlayer->state + 0x34C)) = 0;
    (*(u8*) (pPlayer->state + 0x360)) = 0;
    //void (*func)(
    //    ObjInstance *player, void *state, void *state2) = 0x802b0ea4;
    //func(pPlayer, pPlayer->state, pPlayer->state);
    void (*func)(
        ObjInstance *player, void *state) = 0x802a514c;
    func(pPlayer, pPlayer->state);*/

    adjustPlayerRotation(pad->stickX * 16, pad->stickY * 16, pad);
}

void applyWiimoteToArwing(ObjInstance *arwing,
GameWiimoteState *wp, PADStatus *pad,
u32 *bHeld, u32 *bDown, u32 *bUp) {
    void *state = arwing->state;

    *bHeld = mapWiimoteButtons(wp, wp->btnsHeld);
    *bDown = mapWiimoteButtons(wp, wp->btnsDown);
    *bUp   = mapWiimoteButtons(wp, wp->btnsUp);
    applyJoystickInputs(wp, pad);

    static float prevTilt = 0;
    float tilt =  (wp->orient[2]+prevTilt) / 2.0f;
    prevTilt = tilt;
    tilt *= 2.0f;
    if(ABS(tilt) > 8.0f) {
        if(tilt > 0.0f) pad->triggerRight = MAX(0, MIN(255, tilt));
        else pad->triggerLeft = MAX(0, MIN(255, -tilt));
    }
    if(wp->expType == WPAD_EXP_NUNCHUK && (
    ABS(wp->exp.nunchuk.gforce[0]) +
    ABS(wp->exp.nunchuk.gforce[1]) +
    ABS(wp->exp.nunchuk.gforce[2]) >= 1.1f)) {
        //do a barrel roll! (but actually an aileron roll)
        if(tilt > 0) *bDown |= PAD_TRIGGER_L;
        else *bDown |= PAD_TRIGGER_R;
    }
    /*if(wp->expType == WPAD_EXP_NUNCHUK) {
        *(s16*)(state+0x33A) = wp->exp.nunchuk.orient[2];
        *(s16*)(state+0x33C) = wp->exp.nunchuk.orient[1];
    }*/

    *(float*)(state+0x328) = -69.0f; //disable steering inertia

    //XXX IR aiming; be able to use joystick instead of gyro
}

s8 arwingGetStickXHook(int whichPad) {
    //return padGetStickX(whichPad);
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return padGetStickX(whichPad);
    GameWiimoteState *wp = &wii->wiimote[whichPad];
    if(!(wp->flags & WM_FLAG_WORKING)) {
        return padGetStickX(whichPad);
    }

    static float prevTilt = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            //float tilt = (wp->exp.nunchuk.orient[2]+prevTilt) / 2.0f;
            float tilt = wp->exp.nunchuk.orient[2];
            prevTilt = tilt;
            return MAX(-127, MIN(127, tilt * 2.0f));
        }
        default: return padGetStickX(whichPad);
    }
}
s8 arwingGetStickYHook(int whichPad) {
    //return padGetStickY(whichPad);
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return padGetStickY(whichPad);
    GameWiimoteState *wp = &wii->wiimote[whichPad];
    if(!(wp->flags & WM_FLAG_WORKING)) {
        return padGetStickY(whichPad);
    }

    static float prevTilt = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            //float tilt = (wp->exp.nunchuk.orient[1]+prevTilt) / 2.0f;
            float tilt = wp->exp.nunchuk.orient[1];
            prevTilt = tilt;
            return MAX(-127, MIN(127, tilt * 2.0f));
        }
        default: return padGetStickY(whichPad);
    }
}

void applyWiimoteToPlayer(GameWiimoteState *wp, PADStatus *pad,
u32 *bHeld, u32 *bDown, u32 *bUp) {
    bool isAim = applyAimToStaff(wp, pad);

    *bHeld = mapWiimoteButtons(wp, wp->btnsHeld);
    *bDown = mapWiimoteButtons(wp, wp->btnsDown);
    *bUp   = mapWiimoteButtons(wp, wp->btnsUp);

    if((*bHeld | *bDown) & PAD_TRIGGER_R) {
        //the digital input doesn't trigger the shield
        pad->triggerRight = 255;
    }
    applyJoystickInputs(wp, pad);
    if(isAim) {
        //HACK: don't calculate staff aim coordinates
        WRITE32(0x8029b1fc, 0x60000000);
        WRITE32(0x8029b270, 0x60000000);
        WRITE32(0x8029b238, 0x60000000);
    }
    doSwingGestures(wp, pad, bDown);
    if(isAim) doAimControls(pad);

}

static u8 prevWiimoteFlags[4];
void applyWiimoteInputs(int iPad, PADStatus *pad) {
    //only called when the Wiimote is connected.
    //overrides the GC controller state.
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = &wii->wiimote[iPad];
    u8 nowFlags = wp->flags & (WM_FLAG_PRESENT | WM_FLAG_WORKING);
    if(nowFlags != prevWiimoteFlags[iPad]) {
        u8 prevFlags = prevWiimoteFlags[iPad];
        if((  nowFlags & WM_FLAG_PRESENT)
        && !(prevFlags & WM_FLAG_PRESENT)) {
            //XXX on-screen display
            if(nowFlags & WM_FLAG_WORKING) {
                OSReport("Wiimote connected\n");
            }
            else OSReport("Wiimote connecting...\n");
        }
        else OSReport("Wiimote disconnected\n");
        prevWiimoteFlags[iPad] = nowFlags;
    }
    if(!(nowFlags & WM_FLAG_WORKING)) return;

    u32 bHeld = pad->button;
    u32 bDown = pad->button;
    u32 bUp   = 0;

    ObjInstance *arwing = getArwing();
    if(arwing) applyWiimoteToArwing(arwing, wp, pad,
        &bHeld, &bDown, &bUp);
    else applyWiimoteToPlayer(wp, pad, &bHeld, &bDown, &bUp);

    displayWiimoteState(wp);
    pad->button = bHeld | bDown;
}

u32 padUpdate_hook(PADStatus *state) {
    //replaces call to padReadControllers()

    //HACK: restore aiming calculations.
    //will be NOP'd if we're using Wiimote aiming.
    WRITE32(0x8029b1fc, 0xd01f0788);
    WRITE32(0x8029b270, 0xd01f078c);
    WRITE32(0x8029b238, 0xd01f078c);

    //call original method
    u32 (*padReadControllers)(PADStatus*) = 0x8024e864;
    u32 result = padReadControllers(state);
    if(!IS_WII) return result;

    GameWiiInterface *wii = WII_IFACE_PTR;
    if(wii->magic != WII_IFACE_MAGIC) {
        //not ready yet
        memset(prevWiimoteFlags, 0, sizeof(prevWiimoteFlags));
        return result;
    }

    state->err = 0; //present, even if no GC pad connected
    wii->updateWiimotes();
    applyWiimoteInputs(0, state);
    displayControllerState(0, state);
    return result;
}

int padGetCxHook(int pad) {
    //replaces the body of padGetCX().
    if(joypadDisable || isDvdDriveBusy) return 0;
    if((pad == 0) && !(cameraFlags & CAM_FLAG_PAD3)) {
        //we use C stick to move camera
        //this means we don't want other things to react to the C stick
        u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;
        if(buttons & PAD_BUTTON_LEFT)  return -16;
        if(buttons & PAD_BUTTON_RIGHT) return  16;
        return 0;
    }
    return controllerStates[pad + (whichControllerFrame * 4)].substickX;
}

int padGetCyHook(int pad) {
    //replaces the body of padGetCY().
    if(joypadDisable || isDvdDriveBusy) return 0;
    if((pad == 0) && !(cameraFlags & CAM_FLAG_PAD3)) {
        u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;
        if(buttons & PAD_BUTTON_UP)   return  16;
        if(buttons & PAD_BUTTON_DOWN) return -16;
        return 0;
    }
    return controllerStates[pad + (whichControllerFrame * 4)].substickY;
}

//XXX check if cannon is inverted by default like viewfinder is.
int padGetStickXHook(int pad) {
    //replaces the body of padGetStickX().
    if(joypadDisable || isDvdDriveBusy) return 0;
    int result = controllerStates[pad + (whichControllerFrame * 4)].stickX;
    u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;

    //mode 0x52 is both holding L button and aiming with staff,
    //and pushing blocks.
    void *pState = pPlayer ? pPlayer->state : NULL;
    u16 stateNo = pState ? *(u16*)(pState + 0x274) : 0;
    //don't invert controls when pushing a block
    if(stateNo == 0x1D) return result;

    if((cameraFlags & CAM_FLAG_INVERT_X) && !(buttons & PAD_TRIGGER_L)) {
        if(cameraMode == 0x52    //staff aiming
        || cameraMode == 0x44    //viewfinder
        || cameraMode == 0x51) { //aim cannon
            return -result;
        }
    }
    return result;
}

int padGetStickYHook(int pad) {
    //replaces the body of padGetStickY().
    if(joypadDisable || isDvdDriveBusy) return 0;
    int result = controllerStates[pad + (whichControllerFrame * 4)].stickY;
    u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;

    void *pState = pPlayer ? pPlayer->state : NULL;
    u16 stateNo = pState ? *(u16*)(pState + 0x274) : 0;
    //don't invert controls when pushing a block
    if(stateNo == 0x1D) return result;

    if(cameraMode == 0x44) { //viewfinder
        //already inverted by default, so do opposite
        if(!(cameraFlags & CAM_FLAG_INVERT_Y)) return -result;
    }
    else if((cameraFlags & CAM_FLAG_INVERT_Y) && !(buttons & PAD_TRIGGER_L) && (
    cameraMode == 0x52 || cameraMode == 0x51)) {
        return -result;
    }
    return result;
}

void wiiControlsInit() {
    hookBranch(0x80014f90, padUpdate_hook, 1);
    hookBranch(0x8022a6bc, arwingGetStickXHook, 1);
    hookBranch(0x8022a6f0, arwingGetStickYHook, 1);
}
