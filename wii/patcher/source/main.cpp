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
static bool bExit = false;

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
            printf("STM power off\r\n");
            STM_ShutdownToStandby();
            break;

        case STM_EVENT_RESET:
            printf("STM reboot\r\n");
            STM_RebootSystem();
            break;

        default:
            printf("STM unknown event %d\r\n", event);
            return;
    }

    //fatUnmount("sd");
    //we're jumping to null sometime after this function (Dolphin bug?)
    bExit = true;
}

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    SET_DISC_LED(1);
    int err = 0;
    err = appGxInit();
    if(err) return err;

    initDebugPrint();
    fprintf(stderr, " *** debug print online ***\r\n");
    appFontInit();
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

static void drawBackground(GX::Texture *tex) {
    u16 screenW, screenH, texW, texH;
    appGxGetScreenSize(&screenW, &screenH);
    tex->getSize(&texW, &texH);
    appDrawSprite(tex, (screenW/2)-(texW/2), (screenH/2)-(texH/2), 1.0f);
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
    printf("This is item one\r\n");
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
    try {
        GX::Texture texTest(root / "res/test.tex");
        GX::Texture texCursor(root / "res/generic_point.tex");
        auto *texButton = new GX::Texture(root / "res/button.tex");

        auto item = new UI::MenuItem("First Item", MenuItemOneActivate);
        item->setBgTexture(texButton);
        mainMenu.addItem(item);

        item = new UI::MenuItem("Item the Second", nullptr);
        item->setBgTexture(texButton);
        mainMenu.addItem(item);

        item = new UI::MenuItem("here's three!", nullptr);
        item->setBgTexture(texButton)->setEnabled(false);
        mainMenu.addItem(item);

        item = new UI::MenuItem("This item's text is way too long what the hell kind of menu item even is this supposed to be", nullptr);
        item->setBgTexture(texButton);
        mainMenu.addItem(item);

        int irX=0, irY=0;
        while(!bExit) {
            appGxFrameBegin();
            drawBackground(&texTest);

            PAD_ScanPads();
            u32 wpadButtonsDown = updateWpad(&irX, &irY);
            if(wpadButtonsDown & WPAD_BUTTON_HOME) exit(0);
            if(wpadButtonsDown & WPAD_BUTTON_DOWN) mainMenu.move( 1);
            if(wpadButtonsDown & WPAD_BUTTON_UP)   mainMenu.move(-1);
            if(wpadButtonsDown & WPAD_BUTTON_A)    mainMenu.select();

            //cursor is offset
            appDrawSprite(&texCursor, irX-48, irY-48, 1.0f);

            char msg[256];
            sprintf(msg, "cursor %d %d", irX, irY);
            fontSetSize(16);
            fontSetPos(0, 16);
            fontSetColor({255, 255, 255, 255});
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
