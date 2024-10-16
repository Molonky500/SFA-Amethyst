/** Debug Object List submenu for filtering by type.
 */
#include "main.h"
#include "menuDebugObjs.h"

#define OBJTYPE_MENU_XPOS 15
#define OBJTYPE_MENU_YPOS 50
#define OBJTYPE_MENU_WIDTH  600
#define OBJTYPE_MENU_HEIGHT 405
#define OBJTYPE_MENU_NUM_LINES ((OBJTYPE_MENU_HEIGHT / LINE_HEIGHT) - 2)

void objTypeMenu_draw(Menu *self) {
    //Draw function for Object Type menu
    menuAnimFrame++;
    drawMenuBox(OBJTYPE_MENU_XPOS, OBJTYPE_MENU_YPOS,
        OBJTYPE_MENU_WIDTH, OBJTYPE_MENU_HEIGHT);

    int x = OBJTYPE_MENU_XPOS + MENU_PADDING;
    int y = OBJTYPE_MENU_YPOS + MENU_PADDING;
    int start = MAX(0, self->selected - (OBJTYPE_MENU_NUM_LINES-1));

    char str[256];
    for(int i=0; i < OBJTYPE_MENU_NUM_LINES; i++) {
        int idx = i + start;
        if(idx >= MAX_OBJTYPES + 1) break;
        if(idx == 0) sprintf(str, "\eF%s (%d)", T("All"), numLoadedObjs);
        else {
            const char *name = objTypeNames[idx-1];
            int nObjs = 0;
            objGetObjsType(idx-1, &nObjs);
            if(!name) name = "?";
            sprintf(str, "\eF%02X %-30s (%d)", idx-1, name, nObjs);
        }

        Color4b color = {.r=192, .g=192, .b=192, .a=255};
        if(idx == self->selected) {
            color = hsv2rgb(menuTextAnimFrame*2, 192, 192, 128);
        }

        drawColorText(str, x, y, color);
        y += LINE_HEIGHT;
    }
}

void objTypeMenu_close(Menu *self) {
    menuInputDelayTimer = MENU_INPUT_DELAY_CLOSE;
    audioPlaySound(NULL, MENU_OPEN_SOUND);
    self->close(curMenu);
}

void objTypeMenu_run(Menu *self) {
    //Run function for Object Type menu
    int sel = self->selected;

    if(buttonsJustPressed == PAD_BUTTON_A) {
        debugObjList_whichList = self->selected - 1;
        objTypeMenu_close(self);
    }
    //XXX find some way to avoid repeating this
    else if(buttonsJustPressed == PAD_BUTTON_B) {
        objTypeMenu_close(self);
    }
    else if(controllerStates[0].stickY > MENU_ANALOG_STICK_THRESHOLD
    ||      controllerStates[0].substickY > MENU_CSTICK_THRESHOLD) { //up
        menuInputDelayTimer =
            (controllerStates[0].stickY > MENU_ANALOG_STICK_THRESHOLD)
            ? MENU_INPUT_DELAY_MOVE : MENU_INPUT_DELAY_MOVE_FAST;
        if(sel == 0) sel = MAX_OBJTYPES+1;
        self->selected = sel - 1;
    }
    else if(controllerStates[0].stickY < -MENU_ANALOG_STICK_THRESHOLD
    ||      controllerStates[0].substickY < -MENU_CSTICK_THRESHOLD) { //down
        menuInputDelayTimer = (controllerStates[0].stickY < -MENU_ANALOG_STICK_THRESHOLD)
            ? MENU_INPUT_DELAY_MOVE : MENU_INPUT_DELAY_MOVE_FAST;
        sel++;
        if(sel > MAX_OBJTYPES) sel = 0;
        self->selected = sel;
    }
    else if(controllerStates[0].triggerLeft > MENU_TRIGGER_THRESHOLD) { //L
        sel -= OBJ_MENU_NUM_LINES;
        if(sel <= 0) sel = MAX_OBJTYPES+1;
        self->selected = sel - 1;
        menuInputDelayTimer = MENU_INPUT_DELAY_MOVE;
    }
    else if(controllerStates[0].triggerRight > MENU_TRIGGER_THRESHOLD) { //R
        sel += OBJ_MENU_NUM_LINES;
        if(sel > MAX_OBJTYPES) sel = 0;
        self->selected = sel;
        menuInputDelayTimer = MENU_INPUT_DELAY_MOVE;
    }
}

Menu menuDebugObjType = {
    "Object Type", 0,
    objTypeMenu_run, objTypeMenu_draw, objListSubmenu_close,
    NULL,
};
