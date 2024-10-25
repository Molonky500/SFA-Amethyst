#include "main.h"
#include "Menu.h"
#include "Texture.h"

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;

u32 gFrameCount = 0;

uint32_t crc32b(const void *data_, uint32_t len) {
    const uint8_t *data = (const uint8_t*)data_;
    unsigned int crc, mask;

    crc = 0xFFFFFFFF;
    for(uint32_t i=0; i<len; i++) {
        crc = crc ^ data[i];
        for(uint32_t j = 0; j < 8; j++) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

void MyStmHandler(u32 event) {
    switch(event) {
        case STM_EVENT_POWER:
            STM_ShutdownToStandby();
            break;

        case STM_EVENT_RESET:
            STM_RebootSystem();
            break;
    }
}

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    SET_DISC_LED(1);
    int err = 0;
    err = appGxInit();
    if(err) return err;

    //debug print must init after font because it
    //somehow interferes with EXI access to load fonts
    //but this way breaks debug output :|
    appFontInit();
    initDebugPrint();
    fprintf(stderr, " *** debug print online ***\r\n");

    WPAD_Init();
    PAD_Init();
    STM_RegisterEventHandler(MyStmHandler);
    if(!fatInitDefault()) {
        fprintf(stderr, "FAT init failed\r\n");
        return 1;
    }

    printf("Startup OK\r\n");
    SET_DISC_LED(0);
    return 0;
}

static AppVtx vtxsCursor[] = {
    //gcc insists the color be wrapped in THREE sets of braces
    //x, y, r, g, b, a, s, t
    { 0,  0, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0000, 0x0000}, //UL
    {16,  0, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0100, 0x0000}, //UR
    {16, 16, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0100, 0x0100}, //BR
    { 0, 16, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0000, 0x0100}, //BL
};
static void drawCursor(int x, int y) {
    //TODO: texture
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    for(int i=0; i<4; i++) {
        AppVtx *vtx = &vtxsCursor[i];
        GX_Position2s16(vtx->x + x, vtx->y + y);
        GX_Color4u8(vtx->c.r, vtx->c.g, vtx->c.b, vtx->c.a);
        GX_TexCoord2s16(vtx->s, vtx->t);
    }
    GX_End();
}

static void drawBackground(GX::Texture *tex) {
    static AppVtx vtxs[] = {
        //x, y, r, g, b, a, s, t
        {0, 0, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0000, 0x0000}, //UL
        {1, 0, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0100, 0x0000}, //UR
        {1, 1, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0100, 0x0100}, //BR
        {0, 1, {{{0xFF, 0xFF, 0xFF, 0xFF}}}, 0x0000, 0x0100}, //BL
    };
    u16 screenW, screenH;
    appGxGetScreenSize(&screenW, &screenH);
    tex->select();
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    for(int i=0; i<4; i++) {
        AppVtx *vtx = &vtxs[i];
        GX_Position2s16(vtx->x * screenW, vtx->y * screenH);
        GX_Color4u8(vtx->c.r, vtx->c.g, vtx->c.b, vtx->c.a);
        GX_TexCoord2s16(vtx->s, vtx->t);
    }
    GX_End();
}

u32 updateWpad(int *outIrX, int *outIrY) {
    WPADData *wpad;
    WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, 640 + 128, 480 + 128);
    WPAD_ScanPads();
    wpad = WPAD_Data(0);

    if(wpad->ir.smooth_valid) {
        if(outIrX) *outIrX = wpad->ir.sx;
        if(outIrY) *outIrY = wpad->ir.sy;
    }
    else if(wpad->ir.raw_valid) {
        if(outIrX) *outIrX = wpad->ir.ax;
        if(outIrY) *outIrY = wpad->ir.ay;
    }

    return WPAD_ButtonsDown(0);
}

void MenuItemOneActivate(UI::MenuItem *item) {
    exit(0);
}

int main(int argc, char **argv) {
    _ipcReg[0x64>>2]  = 0xFFFFFFFF; //AHBPROT: access everything
    _ipcReg[0xFC>>2] |= 0x00FF0020; //allow PPC access
    _ipcReg[0xDC>>2]  = 0xFFFFFFFF; //enable GPIOs
    _ipcReg[0xC4>>2] |= 0x00FF0020; //set directions

    int err = init();
    if(err) {
        fprintf(stderr, "Startup failed\r\n");
        exit(1);
    }

    std::filesystem::path root;
    if(argv) root = argv[0];
    else root = "sd:/apps/patcher/";

    UI::Menu mainMenu;
    mainMenu.addItem(new UI::MenuItem("First Item", MenuItemOneActivate));
    mainMenu.addItem(new UI::MenuItem("Item the Second", nullptr));
    mainMenu.addItem(new UI::MenuItem("here's three!", nullptr));

    try {
        GX::Texture texTest(root / "res/test.tex");
        u16 texW=0, texH=0;
        texTest.getSize(&texW, &texH);
        fprintf(stderr, "Texture size %dx%d\r\n", texW, texH);

        int irX=0, irY=0;
        while(1) {
            appGxFrameBegin();
            drawBackground(&texTest);

            PAD_ScanPads();
            u32 wpadButtonsDown = updateWpad(&irX, &irY);
            if(wpadButtonsDown & WPAD_BUTTON_HOME) exit(0);
            if(wpadButtonsDown & WPAD_BUTTON_DOWN) mainMenu.move( 1);
            if(wpadButtonsDown & WPAD_BUTTON_UP)   mainMenu.move(-1);
            if(wpadButtonsDown & WPAD_BUTTON_A)    mainMenu.select();

            drawCursor(irX, irY);

            char msg[256];
            sprintf(msg, "root=%s", root.c_str());
            fontSetSize(16);
            fontSetPos(20, 260);
            fontSetColor(hsv2rgb(gFrameCount, 255, 255, 255));
            fontDrawString(msg);

            mainMenu.draw();

            appGxFrameEnd();
            gFrameCount++;
        }
    }
    catch(std::exception &ex) {
        fprintf(stderr, "SOFTWARE FAILURE: %s\r\n", ex.what());
        exit(1);
    }
    catch(std::exception *ex) {
        fprintf(stderr, "SOFTWARE FAILURE: %s\r\n", ex->what());
        exit(1);
    }
	return 0;
}
