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

void displayWiimoteState(int iPad, GameWiimoteState *wp) {
    debugPrintf(DPRINT_FIXED " B%3d%% F%04X IR%d, %d, %dcm ang%d ",
        (int)(((float)wp->battery / 255.0f) * 100.0f), wp->flags,
        (int)wp->ir[0], (int)wp->ir[1], (int)(wp->ir[2] * 100.0f),
        (int)(wp->irAngle));
    debugPrintf("A " DPRINT_FIXED "%3d,%3d,%3d" DPRINT_NOFIXED " ",
        (int)wp->accel[0], (int)wp->accel[1], (int)wp->accel[2]);
    debugPrintf("O " DPRINT_FIXED "%3d,%3d,%3d" DPRINT_NOFIXED " ",
        (int)wp->orient[0], (int)wp->orient[1], (int)wp->orient[2]);
    debugPrintf("G " DPRINT_FIXED "%3d,%3d,%3d = %d" DPRINT_NOFIXED " ",
        (int)wp->gforce[0], (int)wp->gforce[1], (int)wp->gforce[2],
        (int)(ABS(wp->gforce[0])+ABS(wp->gforce[1])+ABS(wp->gforce[2]))
        );

    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            vec3f orient;
            wiiGetNunchukNormalizedOrient(iPad, &orient);
            debugPrintf("\nNC: J " DPRINT_FIXED "%3d,%3d" DPRINT_NOFIXED
            " Acel " DPRINT_FIXED "%3d,%3d,%3d" DPRINT_NOFIXED " ",
                wp->exp.nunchuk.joystick[0],
                wp->exp.nunchuk.joystick[1],
                (int)wp->exp.nunchuk.accel[0],
                (int)wp->exp.nunchuk.accel[1],
                (int)wp->exp.nunchuk.accel[2]);
            debugPrintf("Ornt " DPRINT_FIXED "%4d,%4d,%4d" DPRINT_NOFIXED " ",
                //(int)wp->exp.nunchuk.orient[0],
                //(int)wp->exp.nunchuk.orient[1],
                //(int)wp->exp.nunchuk.orient[2]
                (int)(orient.x * 100.0f),
                (int)(orient.y * 100.0f),
                (int)(orient.z * 100.0f));
            debugPrintf("Gfrc " DPRINT_FIXED "%3d,%3d,%3d = %d" DPRINT_NOFIXED "\n",
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

void adjustPlayerRotation(int iPad, s16 x, s16 y, PADStatus *pad) {
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
        //XXX EarthWalker
    }
}

u32 mapNunchukButtons(int iPad, GameWiimoteState *wp, u32 btns) {
    u32 result = 0;
    if(btns & WPAD_NUNCHUK_BUTTON_Z) result |= PAD_TRIGGER_L;
    if(btns & WPAD_NUNCHUK_BUTTON_C) result |= PAD_BUTTON_Y;
    return result;
}

u32 mapClassicButtons(int iPad, GameWiimoteState *wp, u32 btns) {
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

u32 mapWiimoteButtons(int iPad, GameWiimoteState *wp, u32 btns) {
    //generate GC button mask corresponding to Wii button
    //state on given pad.
    //mapping chosen arbitrarily to fit the game.
    u32 result = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            result |= mapNunchukButtons(iPad, wp, btns);
            break;
        }
        case WPAD_EXP_CLASSIC: {
            result |= mapClassicButtons(iPad, wp, btns);
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

static float prevAimX = SCREEN_WIDTH  / 2.0f;
static float prevAimY = SCREEN_HEIGHT / 2.0f;
bool applyAimToStaff(int iPad, GameWiimoteState *wp, PADStatus *pad) {
    if(!pPlayer) return false;
    //don't apply to title screen Fox, Arwing, etc
    if(pPlayer->catId != ObjCatId_Player) return false;
    void *state = pPlayer->state;
    if(!state) return false;

    float x = wp->ir[0] - 320.0f;
    float y = wp->ir[1] - 240.0f;
    if(debugTextFlags & DEBUGTEXT_WIIMOTE) {
        debugPrintf("IR AIM: %4d, %4d (%4d, %4d) %04X\n", (int)x, (int)y,
            (int)prevAimX, (int)prevAimY, wp->flags);
    }
    //XXX apparently this flag isn't set when aiming
    //toward the right edge of the screen, even though
    //it works just fine...
    if(!(wp->flags & WM_FLAG_IR_VALID)) {
        x = prevAimX;
        y = prevAimY;
    }
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
            *(float*)(state+0x7B8) = CLAMP(x/640.0f, -2.0f, 2.0f);
            *(float*)(state+0x7BC) = CLAMP(y/480.0f, -2.0f, 2.0f);
            return true; //we are in an aiming state
        }
        default: {
            //adjust the physical staff motion
            //this also affects the camera...
            // *(float*)(state+0x7B8) = CLAMP(((x/640.0f)-0.75f)*2.0f, -1.0f, 1.0f);
            // *(float*)(state+0x7BC) = CLAMP((y/480.0f)-0.5f, -1.0f, 1.0f);
            *(float*)(state+0x7B8) = CLAMP(x/640.0f, -2.0f, 2.0f);
            *(float*)(state+0x7BC) = CLAMP(y/480.0f, -2.0f, 2.0f);
            return false;
        }
    }
}

void applyJoystickInputs(int iPad, GameWiimoteState *wp, PADStatus *pad) {
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            pad->stickX = CLAMP(wp->exp.nunchuk.joystick[0] * 2, -127, 127);
            pad->stickY = CLAMP(wp->exp.nunchuk.joystick[1] * 2, -127, 127);
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

void doSwingGestures(int iPad, GameWiimoteState *wp, PADStatus *pad, u32 *bDown) {
    vec3f orient;
    if((!pPlayer) || pPlayer->catId != ObjCatId_Player) return;
    s16 stateNo = *(s16*)(pPlayer->state+0x274);

    //swing to roll
    if((wiimoteCfg[iPad].options & WII_SHAKE_TO_ROLL) &&
    wp->expType == WPAD_EXP_NUNCHUK && (
    ABS(wp->exp.nunchuk.gforce[0]) +
    ABS(wp->exp.nunchuk.gforce[1]) +
    ABS(wp->exp.nunchuk.gforce[2]) >= 1.1f)) {
        OSReport("rollin' rollin' rollin'\n");
        *bDown |= PAD_BUTTON_X;
    }

    switch(stateNo) {
        case 0x01: //idle
        case 0x02: //moving
        case 0x24: //idle in combat
        case 0x25: { //moving in combat
            //swing to attack
            if(*(u8*)(pPlayer->state+0x8B3) != 0) { //staff in hand
                if((wiimoteCfg[iPad].options & WII_SHAKE_TO_SWING) &&
                ABS(wp->gforce[0]) +
                ABS(wp->gforce[1]) +
                ABS(wp->gforce[2]) >= 6.0f) {
                    OSReport("swing it!\n");
                    *bDown |= PAD_BUTTON_A;
                }
            }
            break;
        }
        case 0x18: { //riding a bike/CloudRunner/mammoth
            if(wp->expType == WPAD_EXP_NUNCHUK
            && (wiimoteCfg[iPad].options & WII_NUNCHUK_STEER)) {
                wiiGetNunchukNormalizedOrient(iPad, &orient);
                adjustPlayerRotation(iPad, orient.z * 12, orient.y * 10, pad);
                //maybe forward tilt can be accel/brake instead
            }
            break;
        }
        case 0x1A: { //riding CloudRunner (DragRock?)
            if(wp->expType == WPAD_EXP_NUNCHUK
            && (wiimoteCfg[iPad].options & WII_NUNCHUK_STEER)) {
                wiiGetNunchukNormalizedOrient(iPad, &orient);
                pad->stickX += orient.z;
                pad->stickY += orient.y;
            }
            break;
        }
        default: break;
    }
}

void doAimControls(int iPad, PADStatus *pad) {
    if((!pPlayer) || pPlayer->catId != ObjCatId_Player) return;
    //try to let player walk around in this state.
    //this doesn't work...
    //can we instead put them in "holding L" state
    //but still have them use spells instead of swinging?

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

    //XXX this should also tilt the camera up/down
    adjustPlayerRotation(iPad, pad->stickX * 16, pad->stickY * 16, pad);
}

void applyWiimoteToArwing(int iPad, ObjInstance *arwing,
GameWiimoteState *wp, PADStatus *pad,
u32 *bHeld, u32 *bDown, u32 *bUp) {
    //XXX this is using the Wiimote itself, not the Nunchuk;
    //should be a separate flag
    if(!(wiimoteCfg[iPad].options & WII_NUNCHUK_STEER)) return;
    void *state = arwing->state;
    *bHeld = mapWiimoteButtons(iPad, wp, wp->btnsHeld);
    *bDown = mapWiimoteButtons(iPad, wp, wp->btnsDown);
    *bUp   = mapWiimoteButtons(iPad, wp, wp->btnsUp);
    applyJoystickInputs(iPad, wp, pad);

    static float prevTilt = 0;
    float tilt =  (wp->orient[2]+prevTilt) / 2.0f;
    prevTilt = tilt;
    tilt *= 2.0f;
    if(ABS(tilt) > 8.0f) {
        if(tilt > 0.0f) pad->triggerRight = CLAMP(tilt, 0, 255);
        else pad->triggerLeft = CLAMP(-tilt, 0, 255);
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

s8 arwingGetStickXHook(int iPad) {
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return padGetStickX(iPad);
    if(!(wiimoteCfg[iPad].options & WII_NUNCHUK_STEER)) return padGetStickX(iPad);
    GameWiimoteState *wp = &wii->wiimote[iPad];
    if(!(wp->flags & WM_FLAG_WORKING)) {
        return padGetStickX(iPad);
    }

    static float prevTilt = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            //float tilt = (wp->exp.nunchuk.orient[2]+prevTilt) / 2.0f;
            vec3f orient;
            if(!wiiGetNunchukNormalizedOrient(iPad, &orient)) {
                return padGetStickX(iPad);
            }
            float tilt = orient.x;
            prevTilt = tilt;
            return CLAMP(tilt * 2.0f, -127, 127);
        }
        default: return padGetStickX(iPad);
    }
}
s8 arwingGetStickYHook(int iPad) {
    GameWiiInterface *wii = WII_IFACE_PTR;
    if(!wii) return padGetStickY(iPad);
    if(!(wiimoteCfg[iPad].options & WII_NUNCHUK_STEER)) return padGetStickY(iPad);
    GameWiimoteState *wp = &wii->wiimote[iPad];
    if(!(wp->flags & WM_FLAG_WORKING)) {
        return padGetStickY(iPad);
    }

    static float prevTilt = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            //float tilt = (wp->exp.nunchuk.orient[1]+prevTilt) / 2.0f;
            vec3f orient;
            if(!wiiGetNunchukNormalizedOrient(iPad, &orient)) {
                return padGetStickY(iPad);
            }
            float tilt = orient.y;
            prevTilt = tilt;
            return CLAMP(tilt * 2.0f, -127, 127);
        }
        default: return padGetStickY(iPad);
    }
}

void applyWiimoteToPlayer(int iPad, GameWiimoteState *wp, PADStatus *pad,
u32 *bHeld, u32 *bDown, u32 *bUp) {
    bool isAim = applyAimToStaff(iPad, wp, pad);

    *bHeld = mapWiimoteButtons(iPad, wp, wp->btnsHeld);
    *bDown = mapWiimoteButtons(iPad, wp, wp->btnsDown);
    *bUp   = mapWiimoteButtons(iPad, wp, wp->btnsUp);

    if((*bHeld | *bDown) & PAD_TRIGGER_R) {
        //the digital input doesn't trigger the shield
        pad->triggerRight = 255;
    }
    applyJoystickInputs(iPad, wp, pad);
    if(isAim) {
        //HACK: don't calculate staff aim coordinates
        WRITE32(0x8029b1fc, 0x60000000);
        WRITE32(0x8029b270, 0x60000000);
        WRITE32(0x8029b238, 0x60000000);
    }
    doSwingGestures(iPad, wp, pad, bDown);
    if(isAim) doAimControls(iPad, pad);

}

static u8  prevWiimoteFlags[4] = {0};
static u32 wiimoteBatteryMsgTimer[4] = {0};
static char wiimoteMsg[4][32];
void applyWiimoteInputs(int iPad, PADStatus *pad) {
    //only called when the Wiimote is connected.
    //overrides the GC controller state.
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = &wii->wiimote[iPad];
    u8 nowFlags  = wp->flags & (WM_FLAG_PRESENT | WM_FLAG_WORKING);
    u8 prevFlags = prevWiimoteFlags[iPad];
    u8 setFlags  = nowFlags & ~prevFlags;
    u8 clrFlags  = prevFlags & ~nowFlags;
    if(setFlags || clrFlags) {
        if(setFlags & WM_FLAG_WORKING) {
            //using this buffer lets us add the number and means
            //previous messages will be replaced.
            sprintf(wiimoteMsg[iPad], "Wii Remote %d connected", iPad+1);
            addOsdMessage(wiimoteMsg[iPad], 300);
            OSReport("Wiimote connected\n");
            PADControlMotor(0, 2); //stop motor
            wiimoteBatteryMsgTimer[iPad] = 0;
        }
        else if(setFlags & WM_FLAG_PRESENT) {
            sprintf(wiimoteMsg[iPad], "Wii Remote %d connecting...", iPad+1);
            addOsdMessage(wiimoteMsg[iPad], 180);
            OSReport("Wiimote connecting...\n");
        }
        else if(clrFlags) {
            sprintf(wiimoteMsg[iPad], "Wii Remote %d disconnected", iPad+1);
            addOsdMessage(wiimoteMsg[iPad], 300);
            OSReport("Wiimote disconnected\n");
        }
    }
    prevWiimoteFlags[iPad] = nowFlags;
    if(!(nowFlags & WM_FLAG_WORKING)) {
        //OSReport("buttons=%X\r\n", pad->button);
        return;
    }

    //battery reads 0 at first
    if(wp->battery < 50 && wp->battery > 0) { //0-255
        //OSReport("Battery %d at %d\r\n", iPad, wp->battery);
        if(wiimoteBatteryMsgTimer[iPad]) wiimoteBatteryMsgTimer[iPad]--;
        if(!wiimoteBatteryMsgTimer[iPad]) {
            sprintf(wiimoteMsg[iPad], "Wii Remote %d battery %d%%", iPad+1,
                (int)((float)wp->battery / 2.55f));
            addOsdMessage(wiimoteMsg[iPad], 300);
            wiimoteBatteryMsgTimer[iPad] = 300 * 60; //5 minutes
            audioPlaySound(NULL, 0x38D); //warning beep
        }
    }

    u32 bHeld = pad->button;
    u32 bDown = pad->button;
    u32 bUp   = 0;

    ObjInstance *arwing = getArwing();
    if(arwing) applyWiimoteToArwing(iPad, arwing, wp, pad,
        &bHeld, &bDown, &bUp);
    else applyWiimoteToPlayer(iPad, wp, pad, &bHeld, &bDown, &bUp);

    if(debugTextFlags & DEBUGTEXT_WIIMOTE) {
        displayWiimoteState(iPad, wp);
    }
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
    applyWiimoteInputs(0, state); //XXX ensure this is actually pad 0
    if(debugTextFlags & DEBUGTEXT_WIIMOTE) {
        displayControllerState(0, state);
    }
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
    int result  = controllerStates[pad + (whichControllerFrame * 4)].stickX;
    u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;

    //mode 0x52 is both holding L button and aiming with staff,
    //and pushing blocks.
    void *pState = pPlayer ? pPlayer->state : NULL;
    u16 stateNo = pState ? *(u16*)(pState + 0x274) : 0;
    //don't invert controls when pushing a block
    if(stateNo == 0x1D) return result;

    if((cameraFlags & CAM_FLAG_INVERT_X) && !(buttons & PAD_TRIGGER_L)) {
        if(cameraMode == CAM_DLL_FORCE_BEHIND //staff aiming
        || cameraMode == CAM_DLL_VIEWFINDER   //viewfinder
        || cameraMode == CAM_DLL_CANNON) {    //aim cannon
            return -result;
        }
    }
    return result;
}

int padGetStickYHook(int pad) {
    //replaces the body of padGetStickY().
    if(joypadDisable || isDvdDriveBusy) return 0;
    int result  = controllerStates[pad + (whichControllerFrame * 4)].stickY;
    u16 buttons = controllerStates[pad + (whichControllerFrame * 4)].button;

    void *pState = pPlayer ? pPlayer->state : NULL;
    u16 stateNo = pState ? *(u16*)(pState + 0x274) : 0;
    //don't invert controls when pushing a block
    if(stateNo == 0x1D) return result;

    if(cameraMode == CAM_DLL_VIEWFINDER) {
        //already inverted by default, so do opposite
        if(!(cameraFlags & CAM_FLAG_INVERT_Y)) return -result;
    }
    else if((cameraFlags & CAM_FLAG_INVERT_Y) && !(buttons & PAD_TRIGGER_L) && (
    cameraMode == CAM_DLL_FORCE_BEHIND || cameraMode == CAM_DLL_CANNON)) {
        return -result;
    }
    return result;
}

void PADControlMotor_hook(int chan, int cmd) {
    GameWiiInterface *wii = WII_IFACE_PTR;
    GameWiimoteState *wp = &wii->wiimote[chan];
    if(!(wp->flags & WM_FLAG_WORKING)) {
        //Wiimote not in use
        PADControlMotor(chan, cmd);
        return;
    }
    if(cmd == 1) wp->flags |= WM_FLAG_RUMBLE;
    else wp->flags &= ~WM_FLAG_RUMBLE;
}

void wiiControlsInit() {
    hookBranch(0x80014f90, padUpdate_hook, 1);
    hookBranch(0x8022a6bc, arwingGetStickXHook, 1);
    hookBranch(0x8022a6f0, arwingGetStickYHook, 1);
    hookBranch(0x80014a48, PADControlMotor_hook, 1);
    hookBranch(0x80014a84, PADControlMotor_hook, 1);
    hookBranch(0x80014ad8, PADControlMotor_hook, 1);
    hookBranch(0x80014fec, PADControlMotor_hook, 1);
}
