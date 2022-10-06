#include "main.h"
#include "wiiuse/wpad.h"

bool isWiimoteInit = false;
bool triedWiimoteInit = false;
WPADData *wpads[WPAD_MAX_WIIMOTES];

void __Wpad_PowerCallback(s32 chan) {
    exiPrintf("%s(%d)\n", __FUNCTION__, chan);
}

void initWiimote() {
    if(triedWiimoteInit) return;
    triedWiimoteInit = true;

    int err = CONF_Init();
    exiPrintf("\nCONF_Init: %d\n", err);

    err = WPAD_Init();
    exiPrintf("WPAD_Init: %d\n", err);
    if(err) return;

    int view_width=640, view_height=480;
    for(int i=0; i < WPAD_MAX_WIIMOTES; i++) {
        WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC_IR);
        //WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC);
        //WPAD_SetDataFormat(i, WPAD_FMT_BTNS);
        WPAD_SetVRes(i, view_width + 128, view_height + 128);
    }

    WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);
    WPAD_SetIdleTimeout(120);
    exiPuts("WPAD: initWiimote OK\n");
    isWiimoteInit = true;
}

u32 mapWiimoteButtons(WPADData *wp, u32 btns) {
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

void applyWiimoteInputs(int iPad) {
    static GameControllerState *states = controllerState;
    WPADData *wp = wpads[iPad];
    GameControllerState *control = &states[iPad + (whichControllerFrame*4)];
    u32 bHeld = mapWiimoteButtons(wp, wp->btns_h);
    u32 bDown = mapWiimoteButtons(wp, wp->btns_d);
    u32 bUp   = mapWiimoteButtons(wp, wp->btns_u);
    //XXX where are bHeld, bUp?
    control->buttons = bHeld | bDown;

    //joystick inputs
    switch(wp->exp.type) {
        case WPAD_EXP_NUNCHUK: {
            control->stickX = wp->exp.nunchuk.js.pos.x;
            control->stickY = wp->exp.nunchuk.js.pos.y;
            break;
        }
        default: break;
    }
}

void displayControllerState(int iPad, GameControllerState *cnt) {
    //display input
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
    debugPrintf("%s %3d %3d C%3d %3d X%d\n", msg,
        cnt->stickX, cnt->stickY, cnt->cX, cnt->cY,
        wp ? wp->exp.type : 42069);
}

static int prevState = -1;

void updateWiimote() {
    //if(*(u8*)0x803dd5e8) return; //game is loading
    //if((*(u32*)0x803dd5ec) < 2) return;
    if(!isWiimoteInit) initWiimote();
    if(!isWiimoteInit) return;

    int state = WPAD_GetStatus();
    if(state != WPAD_STATE_ENABLED) {
        if(state == WPAD_STATE_ENABLING) {
            debugPrintf("WPAD: wait for init...\n");
        }
        else {
            debugPrintf("WPAD: disabled\n");
        }
        return;
    }
    if(state != prevState) {
        exiPrintf("WPAD: init OK\n");
        prevState = state;
        /*int view_width=640, view_height=480;
        for(int i=0; i < WPAD_MAX_WIIMOTES; i++) {
            WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC_IR);
            //WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC);
            //WPAD_SetDataFormat(i, WPAD_FMT_BTNS);
            WPAD_SetVRes(i, view_width + 128, view_height + 128);
        }*/
    }

    int err = WPAD_ScanPads();
    if(err < 0) {
        debugPrintf("WP SCAN ERR %d\n", err);
    }

    for(int iPad=0; iPad < WPAD_MAX_WIIMOTES; iPad++) {
        err = WPAD_Probe(iPad, NULL);
        if(err == WPAD_ERR_NONE) {
            debugPrintf("WP%d OK\n", iPad);
			wpads[iPad] = WPAD_Data(iPad);
            applyWiimoteInputs(iPad);
		} else {
            debugPrintf("WP%d err %d\n", iPad, err);
			wpads[iPad] = NULL;
		}
    }
    //WPAD_Flush(WPAD_CHAN_ALL);
}

void padUpdate_hook_() {
    static GameControllerState *states = controllerState;
    GameControllerState *control = &states[0 + (whichControllerFrame*4)];

    /*u8 framesThisStep = *(u8*)0x803db410;
    u8 framesThisTick = *(u8*)0x803db411;
    float msecsThisFrame = *(float*)0x803dccc0;
    exiPrintf("F=%d %d %d\n", framesThisStep, framesThisTick, (int)msecsThisFrame);
    if(!framesThisTick) return;
    if(framesThisStep > 2) return;
    if(msecsThisFrame > 32.0f) return;*/

    //avoid flooding dprint buf
    u32 debugLogEnd = *(u32*)0x803dbc14;
    if(debugLogEnd - 0x803aa018 > 3000) debugLogEnd = 0x803aa018;

    updateWiimote();
    displayControllerState(0, control);
    ipcDebugPrint();
    ipcPumpQueue(); //XXX move
}
void padUpdate_hook(void);
__asm__(
    ".global padUpdate_hook_\n"
    ".global padUpdate_hook \n"
    "padUpdate_hook:        \n"
    ASM_FUNC_START(0x100)
    "stwu    1,  -0x60(1)   \n" //replaced
    //call original padUpdate()
    "lis     12, 0x8001     \n"
    "ori     12, 12, 0x4f44 \n"
    "mtctr   12             \n"
    "bctrl                  \n"
    ASM_FUNC_END(0x100)
    "b padUpdate_hook_      \n"
);
