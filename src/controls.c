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

u32 mapWiimoteButtons(GameWiimoteState *wp, u32 btns) {
    //generate GC button mask corresponding to Wii button
    //state on given pad.
    //mapping chosen arbitrarily to fit the game.
    u32 result = 0;
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
                if(btns & WPAD_NUNCHUK_BUTTON_Z)
                    result |= PAD_TRIGGER_L;
                if(btns & WPAD_NUNCHUK_BUTTON_C)
                    result |= PAD_BUTTON_Y;
                break;
            break;
        }
        case WPAD_EXP_CLASSIC: {
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
            return result; //do not use the regular Wiimote buttons
        }
        default: break;
    }
    if(btns & WPAD_BUTTON_A)     result |= PAD_BUTTON_A;
    if(btns & WPAD_BUTTON_B)     result |= PAD_BUTTON_B;
    if(btns & WPAD_BUTTON_1)     result |= PAD_BUTTON_X;
    if(btns & WPAD_BUTTON_2)     result |= PAD_BUTTON_Y;
    if(btns & WPAD_BUTTON_MINUS) result |= PAD_TRIGGER_Z;
    if(btns & WPAD_BUTTON_PLUS)  result |= PAD_BUTTON_START;
    if(btns & WPAD_BUTTON_HOME)  result |= PAD_BUTTON_START;
    if(btns & WPAD_BUTTON_UP)    result |= PAD_BUTTON_UP;
    if(btns & WPAD_BUTTON_DOWN)  result |= PAD_BUTTON_DOWN;
    if(btns & WPAD_BUTTON_LEFT)  result |= PAD_BUTTON_LEFT;
    if(btns & WPAD_BUTTON_RIGHT) result |= PAD_BUTTON_RIGHT;
    return result;
}

static u8 prevWiimoteFlags[4];
void applyWiimoteInputs(int iPad, PADStatus *state) {
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

    u32 bHeld = mapWiimoteButtons(wp, wp->btnsHeld);
    u32 bDown = mapWiimoteButtons(wp, wp->btnsDown);
    u32 bUp   = mapWiimoteButtons(wp, wp->btnsUp);

    debugPrintf(" B%3d%% IR%d, %d, %dcm ang%d ",
        (int)(((float)wp->battery / 255.0f) * 100.0f),
        (int)wp->ir[0], (int)wp->ir[1], (int)(wp->ir[2] * 100.0f),
        (int)(wp->irAngle));

    //joystick inputs
    switch(wp->expType) {
        case WPAD_EXP_NUNCHUK: {
            debugPrintf("J %3d,%3d\n",
                wp->exp.nunchuk.joystick[0],
                wp->exp.nunchuk.joystick[1]);
            state->stickX = wp->exp.nunchuk.joystick[0];
            state->stickY = wp->exp.nunchuk.joystick[1];
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
            state->stickX       = wp->exp.classic.lStick[0];
            state->stickY       = wp->exp.classic.lStick[1];
            state->substickX    = wp->exp.classic.rStick[0];
            state->substickY    = wp->exp.classic.rStick[1];
            state->triggerLeft  = wp->exp.classic.lTrig;
            state->triggerRight = wp->exp.classic.rTrig;
            break;
        }
        default:
            debugPrintf("\n");
            break;
    }


    //vec3w_t accel (x, y, z)
    //orient_t orient (yaw, pitch, roll)
    //gforce_t gforce (x, y, z)

    state->button = bHeld | bDown;
}

u32 padUpdate_hook(PADStatus *state) {
    //replaces call to padReadControllers()

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

