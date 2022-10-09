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
WPADData *wpads[WPAD_MAX_WIIMOTES];

//XXX properly integrate with OG code
void *pPlayer = (void*)0x803428f8;

void __Wpad_PowerCallback(s32 chan) {
    exiPrintf("%s(%d)\n", __FUNCTION__, chan);
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

void initWiimote() {
    //init if not already tried
    if(triedWiimoteInit) return;
    triedWiimoteInit = true;

    int err = CONF_Init();
    exiPrintf("\nCONF_Init: %d\n", err);
    err = WPAD_Init();
    exiPrintf("WPAD_Init: %d\n", err);
    int view_width=640, view_height=480;
    for(int i=0; i < WPAD_MAX_WIIMOTES; i++) {
        WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC_IR);
        WPAD_SetVRes(i, view_width + 128, view_height + 128);
    }
    WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);
    WPAD_SetIdleTimeout(120);
    exiPuts("WPAD: initWiimote OK\n");
    isWiimoteInit = true;
}

u32 mapWiimoteButtons(WPADData *wp, u32 btns) {
    //generate GC button mask corresponding to Wii button
    //state on given pad.
    //mapping chosen arbitrarily to fit the game.
    u32 result = 0;
    switch(wp->exp.type) {
        case WPAD_EXP_NUNCHUK: {
                if(btns & WPAD_NUNCHUK_BUTTON_Z)
                    result |= PAD_TRIGGER_L;
                if(btns & WPAD_NUNCHUK_BUTTON_C)
                    result |= PAD_BUTTON_Y;
                break;
            break;
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

void applyWiimoteInputs(int iPad, GameControllerState *state) {
    //only called when the Wiimote is connected.
    //overrides the GC controller state.
    WPADData *wp = wpads[iPad];
    u32 bHeld = mapWiimoteButtons(wp, wp->btns_h);
    u32 bDown = mapWiimoteButtons(wp, wp->btns_d);
    u32 bUp   = mapWiimoteButtons(wp, wp->btns_u);
    state->buttons = bHeld | bDown;

    //joystick inputs
    switch(wp->exp.type) {
        case WPAD_EXP_NUNCHUK: {
            state->stickX = wp->exp.nunchuk.js.pos.x + 128;
            state->stickY = wp->exp.nunchuk.js.pos.y + 128;
            break;
        }
        default: break;
    }
}

void displayControllerState(int iPad, GameControllerState *cnt) {
    //display input for debug
    char msg[64];
    char *m = msg;
    *(m++) = (cnt->buttons & PAD_BUTTON_A)     ? 'A' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_B)     ? 'B' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_X)     ? 'X' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_Y)     ? 'Y' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_START) ? 'S' : '.';
    *(m++) = (cnt->buttons & PAD_TRIGGER_L)    ? 'L' : '.';
    *(m++) = (cnt->buttons & PAD_TRIGGER_R)    ? 'R' : '.';
    *(m++) = (cnt->buttons & PAD_TRIGGER_Z)    ? 'Z' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_UP)    ? '^' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_RIGHT) ? '>' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_DOWN)  ? 'V' : '.';
    *(m++) = (cnt->buttons & PAD_BUTTON_LEFT)  ? '<' : '.';
    *(m++) = 0;

    WPADData *wp = wpads[iPad];
    //84=enter fixed-width mode; 85=leave fixed-width mode
    //both = somehow crash
    debugPrintf("%s %3d %3d C%3d %3d", msg,
        cnt->stickX, cnt->stickY, cnt->cX, cnt->cY);
    if(wp) {
        debugPrintf(" B%3d%% IR%d, %d, %dcm ang%d",
            (int)(((float)wp->battery_level / 255.0f) * 100.0f),
            (int)wp->ir.sx,
            (int)wp->ir.sy,
            (int)(wp->ir.z * 100.0f),
            (int)(wp->ir.angle));
        //vec3w_t accel (x, y, z)
        //orient_t orient (yaw, pitch, roll)
        //gforce_t gforce (x, y, z)
    }
    debugPrintf("\n");
}

static int prevState = -1;
static bool checkWpads() {
    initWiimote();
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

void updateWiimote(GameControllerState *state) {
    if(!checkWpads()) return;
    for(int iPad=0; iPad < WPAD_MAX_WIIMOTES; iPad++) {
        int err = WPAD_Probe(iPad, NULL);
        if(err == WPAD_ERR_NONE) {
            debugPrintf("WP%d OK\n", iPad);
			wpads[iPad] = WPAD_Data(iPad);
            applyWiimoteInputs(iPad, &state[iPad]);
		} else {
            debugPrintf("WP%d err %d\n", iPad, err);
			wpads[iPad] = NULL;
		}
    }
}

u32 padUpdate_hook(GameControllerState *state) {
    //replaces call to padReadControllers()

    //call original method
    u32 (*padReadControllers)(GameControllerState*) = 0x8024e864;
    u32 result = padReadControllers(state);
    state->state = 0; //present

    //avoid flooding dprint buf
    //u32 debugLogEnd = *(u32*)0x803dbc14;
    //if(debugLogEnd - 0x803aa018 > 3000) debugLogEnd = 0x803aa018;

    updateWiimote(state);
    displayControllerState(0, state);

    return result;
}
