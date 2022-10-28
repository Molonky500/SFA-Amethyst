/** Debug Object Sequence submenu.
 */
#include "main.h"

#define OBJSEQ_MENU_XPOS     2
#define OBJSEQ_MENU_YPOS     2
#define OBJSEQ_MENU_WIDTH  310
#define OBJSEQ_MENU_HEIGHT 438
#define OBJSEQ_MENU_NUM_LINES ((OBJSEQ_MENU_HEIGHT / LINE_HEIGHT) - 2)
#define OBJSEQ_INFO_XPOS (OBJSEQ_MENU_XPOS + OBJSEQ_MENU_WIDTH)
#define OBJSEQ_INFO_YPOS     2
#define OBJSEQ_INFO_WIDTH  330
#define OBJSEQ_INFO_HEIGHT 438
#define OBJSEQ_INFO_NUM_LINES ((OBJSEQ_INFO_HEIGHT / LINE_HEIGHT) - 1)
#define MAX_SEQ 88
#define MAX_SLOT 26

static const char *seqCondOpNames[] = {
    "?",
    "JUMPTOTIME",
	"COUNTER",
	"COUNTER_ADD",
	"PAUSE",
	"CONTINUE",
	"SubCmd6",
	"MESSAGE",
	"DECISION",
	"JUMPTARGET",
	"JUMPTOLABEL"};

ObjInstance **seqObjs = (ObjInstance**)0x80396918;
s16 *seqIdxs = (s16*)0x8039a3b0; //obj's seqIdx + 1
s32 *seqObjIds = (s32*)0x80399cfc;
SeqBgCmdParam *seqBgCmdParams = (SeqBgCmdParam*)0x80399398;
ObjSeqSubCmdStruct *seqConds = (ObjSeqSubCmdStruct*)0x8039944c;
s8 *seqVarC4C = (s8*)0x80399c4c;
s8 *timeCmdState = (s8*)0x80399ca4;
u8 *flags1 = (u8*)0x80399e50;
u8 *flags2 = (u8*)0x80399ea8;
s16 *xRot = (s16*)0x80399f00;
s16 *timeNext = (s16*)0x80399fac;
float *timeNextF = (float*)0x8039a058;
float *timeCurF = (float*)0x8039a1ac;
s8 *frame = (s8*)0x8039a300;
u8 *unk358 = (u8*)0x8039a358;
u8 *flags3 = (u8*)0x8039a45c;
u8 *unk4B4 = (u8*)0x8039a4b4;
u8 *nextState = (u8*)0x8039a50c;
u8 *curState = (u8*)0x8039a564;
ObjSeqCmd2 *cmds2 = (ObjSeqCmd2*)0x8039a5bc;
s8 *isPaused = (s8*)0x8039a60c;
ObjInstance **objPairs = (ObjInstance**)0x8039a664; //2 per seq

static void drawSeq(int slot) {
    int x = OBJSEQ_INFO_XPOS + MENU_PADDING;
    int y = OBJSEQ_INFO_YPOS + MENU_PADDING;

    ObjInstance *obj1 = seqObjs[slot];
    ObjInstance *obj2 = objPairs[slot*2];
    ObjInstance *obj3 = objPairs[(slot*2)+1];
    char name1[16], name2[16], name3[16];
    getObjName(name1, obj1);
    getObjName(name2, obj2);
    getObjName(name3, obj3);

    Color4b color = {.r=192, .g=192, .b=192, .a=255};
    u32 tFlag = TEXT_COLORED | TEXT_SHADOW;
    char str[1024];
    sprintf(str, "\eF%08X\eF %s\eF\n%08X\eF %s\eF\n%08X\eF %s\eF\n",
        obj1, name1, obj2, name2, obj3, name3);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    sprintf(str, "Idx:\eF%04X\eF ObjId:\eF%08X\n\eF"
        "Time:\eF%4d\eF, \eF%5.1f\eF, \eF%5.1f\n\eF",
        seqIdxs[slot], seqObjIds[slot],
        timeNext[slot], timeNextF[slot], timeCurF[slot]);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    sprintf(str, "Frame:\eF%5d\eF Rot:\eF%04X\eF Pause:%d\eF\n\eF"
        "State:\eF%02X\eF -> \eF%02X\eF (T:%d)\n",
        frame[slot], xRot[slot] & 0xFFFF, isPaused[slot],
        curState[slot], nextState[slot], timeCmdState[slot]);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    sprintf(str, "Flags: \eF%02X %02X %02X\n\eF"
        "C4C:\eF%02X\eF 358:\eF%02X\eF 4B4:\eF%02X\eF\n",
        flags1[slot], flags2[slot], flags3[slot],
        seqVarC4C[slot], unk358[slot], unk4B4[slot]);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    sprintf(str, "BGSeq %d,%d, %d cmds\n",
        seqBgCmdParams[slot].seqNo, seqBgCmdParams[slot].seqNo2,
        seqBgCmdParams[slot].nCmds);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    sprintf(str, "Conds:\eF%08X\eF[%d] next:%d\n",
        seqConds[slot].actions,
        seqConds[slot].nCmds,
        seqConds[slot].nextTime);
    drawText(str, x, y, &x, &y, tFlag, color, 1.0f);

    ObjSeqCondition *conds = seqConds[slot].actions;
    for(int i=0; i<seqConds[slot].nCmds
    && y < OBJSEQ_INFO_YPOS+OBJSEQ_INFO_HEIGHT
    && PTR_VALID(conds); i++) {
        int op = conds[i].indexAndOp & 0x3F;
        int idx = conds[i].indexAndOp >> 6;
        sprintf(str, "\eF%2d\eF: \eF%02X\eF \eF%-12s\eF \eF%04X\eF %d\n",
            i, op,
            op >= 0 && op <= 0xA ? seqCondOpNames[op] : "?",
            conds[i].param & 0xFFFF, idx);
        drawText(str, x, y, &x, &y, tFlag, color, 1.0f);
    }
}

static bool checkClose(Menu *self) {
    if(buttonsJustPressed == PAD_BUTTON_B) { //close menu
        menuInputDelayTimer = MENU_INPUT_DELAY_CLOSE;
        self->close(curMenu);
        return true;
    }
    return false;
}

void objSeqMenu_draw(Menu *self) {
    //Draw function for Object Seq menu
    menuAnimFrame++;

    drawMenuBox(OBJSEQ_MENU_XPOS, OBJSEQ_MENU_YPOS,
        OBJSEQ_MENU_WIDTH, OBJSEQ_MENU_HEIGHT);
    drawMenuBox(OBJSEQ_INFO_XPOS, OBJSEQ_INFO_YPOS,
        OBJSEQ_INFO_WIDTH, OBJSEQ_INFO_HEIGHT);

    int x = OBJSEQ_MENU_XPOS + MENU_PADDING;
    int y = OBJSEQ_MENU_YPOS + MENU_PADDING;
    int start = MAX(0, self->selected - (OBJSEQ_MENU_NUM_LINES-1));

    drawSimpleText("\eFS# Actions #Act Time Idx\eF", x, y);
    y += LINE_HEIGHT;

    char str[256];
    for(int i=0; i < OBJSEQ_MENU_NUM_LINES; i++) {
        int iSeq = i + start;
        if(iSeq >= MAX_SEQ) break;

        Color4b color = {.r=192, .g=192, .b=192, .a=255};
        bool selected = iSeq == self->selected;
        if(selected) {
            drawSeq(iSeq);
            color = hsv2rgb(menuTextAnimFrame*2, 192, 192, 128);
        }

        sprintf(str, "\eF%02X %08X %3d %4d %04X",
            iSeq, seqConds[iSeq].actions, seqConds[iSeq].nCmds,
            seqConds[iSeq].nextTime, seqIdxs[iSeq]);
        drawColorText(str, x, y, color);
        y += LINE_HEIGHT;
    }
}

void objSeqMenu_run(Menu *self) {
    //Run function for Object Seq menu
    int sel = self->selected;

    if(checkClose(self)) {
        //nothing to do
    }
    else if(buttonsJustPressed == PAD_TRIGGER_Z) {
        timeStop = 0; //frame advance
    }
    else if(controllerStates[0].stickY > MENU_ANALOG_STICK_THRESHOLD
    ||      controllerStates[0].substickY > MENU_CSTICK_THRESHOLD) { //up
        menuInputDelayTimer =
            (controllerStates[0].stickY > MENU_ANALOG_STICK_THRESHOLD)
            ? MENU_INPUT_DELAY_MOVE : MENU_INPUT_DELAY_MOVE_FAST;
        if(sel == 0) sel = MAX_SEQ;
        self->selected = sel - 1;
    }
    else if(controllerStates[0].stickY < -MENU_ANALOG_STICK_THRESHOLD
    ||      controllerStates[0].substickY < -MENU_CSTICK_THRESHOLD) { //down
        menuInputDelayTimer = (controllerStates[0].stickY < -MENU_ANALOG_STICK_THRESHOLD)
            ? MENU_INPUT_DELAY_MOVE : MENU_INPUT_DELAY_MOVE_FAST;
        sel++;
        if(sel >= MAX_SEQ) sel = 0;
        self->selected = sel;
    }
    else if(controllerStates[0].triggerLeft > MENU_TRIGGER_THRESHOLD) { //L
        sel -= OBJSEQ_MENU_NUM_LINES;
        if(sel <= 0) sel = MAX_SEQ;
        self->selected = sel - 1;
        menuInputDelayTimer = MENU_INPUT_DELAY_MOVE;
    }
    else if(controllerStates[0].triggerRight > MENU_TRIGGER_THRESHOLD) { //R
        sel += OBJSEQ_MENU_NUM_LINES;
        if(sel >= MAX_SEQ) sel = 0;
        self->selected = sel;
        menuInputDelayTimer = MENU_INPUT_DELAY_MOVE;
    }
}

Menu menuDebugObjSeq = {
    "ObjSeq", 0,
    objSeqMenu_run, objSeqMenu_draw, debugGameSubMenu_close,
    NULL,
};
