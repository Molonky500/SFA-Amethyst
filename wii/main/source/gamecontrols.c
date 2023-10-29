#include "main.h"
#include "wiiuse/wpad.h"

//static u32 *padNewButtons = 0x803398c0;
/*
0x803DC944 SY
0x803DC948 SX
0x803398B0 buttons held
0x803398C0 buttons held
0x803398D0 buttons released
0x803398E0 buttons pressed
0x803398F0 ControllerState
0x803398F2 SX
0x803398F3 SY
0x803398F4 CX
0x803398F5 CY
0x803398F6 L
0x803398F7 R
0x802c6e50 buttonEnableMask
*/

bool isWiimoteInit = false;
bool triedWiimoteInit = false;
WPADData *wpads[GAME_MAX_WIIMOTES];

void __Wpad_PowerCallback(s32 chan) {
    exiPrintf("%s(%d)\n", __FUNCTION__, chan);
    //do nothing. just let the Wiimote turn off.
    //the "correct" thing to do here is to shut off the system.
    //but since this game supports GC controllers too, we
    //want a way to shut off the Wiimote and use those.
    //to actually shut down the system we can use the power
    //button on the system itself, or eventually I'll add
    //an in-game menu option for it.
}

void resetBluetooth() {
    //currently unused, but left for debugging.
    char bluetoothResetData1[] USB_ALIGN = { 0x20 }; //bmRequestType
    char bluetoothResetData2[] USB_ALIGN = { 0x00 }; //bmRequest
    char bluetoothResetData3[] USB_ALIGN = { 0x00, 0x00 }; //wValue
    char bluetoothResetData4[] USB_ALIGN = { 0x00, 0x00 }; //wIndex
    char bluetoothResetData5[] USB_ALIGN = { 0x03, 0x00 }; //wLength
    char bluetoothResetData6[] USB_ALIGN = { 0x00 }; //unknown; set to zero.
    char bluetoothResetData7[] USB_ALIGN = { 0x03, 0x0c, 0x00 }; //Mesage payload.

    /** Vectors of data transfered. */
    ioctlv bluetoothReset[] USB_ALIGN = {
        { bluetoothResetData1, sizeof(bluetoothResetData1) },
        { bluetoothResetData2, sizeof(bluetoothResetData2) },
        { bluetoothResetData3, sizeof(bluetoothResetData3) },
        { bluetoothResetData4, sizeof(bluetoothResetData4) },
        { bluetoothResetData5, sizeof(bluetoothResetData5) },
        { bluetoothResetData6, sizeof(bluetoothResetData6) },
        { bluetoothResetData7, sizeof(bluetoothResetData7) },
    };

    exiPuts("Reset bluetooth...\n");
    int fd = IOS_Open("/dev/usb/oh1/57e/305", IOS_WRITE);
	IOS_Ioctlv(fd, 0, 6, 1, bluetoothReset);
	IOS_Close(fd);
    exiPuts("Reset bluetooth OK\n");
}

int initWiimote() {
    //init if not already tried
    if(triedWiimoteInit) return;
    triedWiimoteInit = true;

    exiPrintf("Init Wiimote...\n");
    //exiPrintf("BT Reset...\n");
    //resetBluetooth();
    //USB_Deinitialize();
    //exiPrintf("WPAD_Shutdown...\n");
    //WPAD_Shutdown();
    //udelay(1000000);

    int err = 0;
    err = CONF_Init();
    exiPrintf("CONF_Init: %d\n", err);
    if(err) return err;
    err = WPAD_Init();
    exiPrintf("WPAD_Init: %d\n", err);
    if(err) return err;

    int view_width=640, view_height=480;
    for(int i=0; i < WPAD_MAX_WIIMOTES; i++) {
        WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC_IR);
        WPAD_SetVRes(i, view_width + 128, view_height + 128);
    }
    WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);
    WPAD_SetIdleTimeout(120);
    exiPuts("WPAD: initWiimote OK\n");
    isWiimoteInit = true;
    return 0;
}

static int prevState = -1;
static bool checkWpads() {
    //initWiimote();
    if(!isWiimoteInit) return false;

    int err = WPAD_ScanPads();
    if(err < 0) {
        debugPrintf("WP SCAN ERR %d\n", err);
        return false;
    }

    int wpadState = WPAD_GetStatus();
    if(wpadState != WPAD_STATE_ENABLED) {
        if(wpadState == WPAD_STATE_ENABLING) {
            debugPrintf("WPAD: wait for init...\n");
        }
        else {
            debugPrintf("WPAD: disabled\n");
        }
        return false;
    }
    if(wpadState != prevState) {
        exiPrintf("WPAD: init OK\n");
        prevState = wpadState;
    }
    return true;
}

void mapJoystick(joystick_t *js, s8 *out) {
    out[0] = (s8)(((float)(js->pos.x - js->center.x) /
        (float)(js->max.x - js->min.x)) * 127.0f);
    out[1] = (s8)(((float)(js->pos.y - js->center.y) /
        (float)(js->max.y - js->min.y)) * 127.0f);
}

void updateGameWiimoteIface(WPADData *pad, int iPad) {
    GameWiimoteState *state = &wiiIface.wiimote[iPad];
    //update the GC-controlled flags
    WPAD_Rumble (iPad, state->flags & WM_FLAG_RUMBLE);
    //WPAD_SetLeds(iPad, state->flags & WM_FLAG_LED_MASK);
    //inexplicably causes constant flashing

    //XXX MotionPlus
    state->battery = pad->battery_level;
    state->btnsDown= pad->btns_d;
    state->btnsHeld= pad->btns_h;
    state->btnsUp  = pad->btns_u;
    state->accel [0] = pad->accel.x;
    state->accel [1] = pad->accel.y;
    state->accel [2] = pad->accel.z;
    state->orient[0] = pad->orient.yaw;
    state->orient[1] = pad->orient.pitch;
    state->orient[2] = pad->orient.roll;
    state->gforce[0] = pad->gforce.x;
    state->gforce[1] = pad->gforce.y;
    state->gforce[2] = pad->gforce.z;
    if(pad->ir.smooth_valid) {
        state->ir[0] = pad->ir.sx;
        state->ir[1] = pad->ir.sy;
    }
    else if(pad->ir.raw_valid) {
        state->ir[0] = pad->ir.ax;
        state->ir[1] = pad->ir.ay;
    }
    //even a single dot is enough for basic motion
    //tracking, though two are needed for angle.
    if(pad->ir.num_dots > 0) state->flags |= WM_FLAG_IR_VALID;
    else state->flags &= ~WM_FLAG_IR_VALID;
    state->ir[2] = pad->ir.z;
    state->irAngle = pad->ir.angle;

    state->expType = pad->exp.type;
    switch(state->expType) {
        case WPAD_EXP_NUNCHUK: {
            state->exp.nunchuk.btnsDown = pad->exp.nunchuk.btns;
            state->exp.nunchuk.btnsHeld = pad->exp.nunchuk.btns_held;
            state->exp.nunchuk.btnsUp   = pad->exp.nunchuk.btns_released;
            mapJoystick(&pad->exp.nunchuk.js, state->exp.nunchuk.joystick);
            state->exp.nunchuk.accel[0] = pad->exp.nunchuk.accel.x;
            state->exp.nunchuk.accel[1] = pad->exp.nunchuk.accel.y;
            state->exp.nunchuk.accel[2] = pad->exp.nunchuk.accel.z;
            state->exp.nunchuk.orient[0] = pad->exp.nunchuk.orient.yaw;
            state->exp.nunchuk.orient[1] = pad->exp.nunchuk.orient.pitch;
            state->exp.nunchuk.orient[2] = pad->exp.nunchuk.orient.roll;
            state->exp.nunchuk.gforce[0] = pad->exp.nunchuk.gforce.x;
            state->exp.nunchuk.gforce[1] = pad->exp.nunchuk.gforce.y;
            state->exp.nunchuk.gforce[2] = pad->exp.nunchuk.gforce.z;
            break;
        }
        case WPAD_EXP_CLASSIC: {
            mapJoystick(&pad->exp.classic.ljs, state->exp.classic.lStick);
            mapJoystick(&pad->exp.classic.rjs, state->exp.classic.rStick);
            state->exp.classic.btnsDown = pad->exp.classic.btns;
            state->exp.classic.btnsHeld = pad->exp.classic.btns_held;
            state->exp.classic.btnsUp = pad->exp.classic.btns_released;
            state->exp.classic.lTrig = (u8)(pad->exp.classic.l_shoulder * 255.0f);
            state->exp.classic.rTrig = (u8)(pad->exp.classic.r_shoulder * 255.0f);
            break;
        }
        default: break;
    }
}

void updateWiimotes() {
    for(int iPad=0; iPad < GAME_MAX_WIIMOTES; iPad++) {
        GameWiimoteState *state = &wiiIface.wiimote[iPad];
        state->flags &= ~(WM_FLAG_PRESENT | WM_FLAG_WORKING);
    }
    if(!checkWpads()) return;

    for(int iPad=0; iPad < GAME_MAX_WIIMOTES; iPad++) {
        GameWiimoteState *state = &wiiIface.wiimote[iPad];
        int err = WPAD_Probe(iPad, NULL);
        if(err == WPAD_ERR_NONE) {
            state->flags |= WM_FLAG_PRESENT | WM_FLAG_WORKING;
            debugPrintf("WP%d OK %02X\n", iPad, state->flags);
			wpads[iPad] = WPAD_Data(iPad);
            updateGameWiimoteIface(wpads[iPad], iPad);
        } else if(err == -1) {
            //not connected; nothing to do here
		} else { //not ready
            state->flags |= WM_FLAG_PRESENT;
            debugPrintf("WP%d err %d\n", iPad, err);
			wpads[iPad] = NULL;
            //wiiIface.wiimote[iPad].flags = 0;
		}
    }
}
