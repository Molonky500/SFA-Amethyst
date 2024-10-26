#include "main.h"
#include "Font.h"
#include "Gx.h"
#include "Menu.h"
#include "Texture.h"
#include "PointerCursor.h"

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;

std::filesystem::path gRootDir;
u32 gFrameCount = 0;
GX::Font *gSystemFont = nullptr;

static bool bExit = false;
static bool bShutDown = false;
static bool bReboot = false;

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
            bShutDown = true;
            break;

        case STM_EVENT_RESET:
            printf("STM reboot\r\n");
            bReboot = true;
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
    err = GX::init();
    if(err) return err;

    initDebugPrint();
    fprintf(stderr, " *** debug print online ***\r\n");
    gSystemFont = new GX::Font();
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

GX::Sprite* loadBackground() {
    auto tex = new GX::Texture(gRootDir / "res/test.tex");
    auto sprBg = new GX::Sprite(tex);

    u16 screenW, screenH, texW, texH;
    GX::getScreenSize(screenW, screenH);
    tex->getSize(&texW, &texH);
    return sprBg->setPos((screenW/2)-(texW/2), (screenH/2)-(texH/2));
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

    if(argv) gRootDir = argv[0];
    else gRootDir = "sd:/apps/patcher/";

    UI::Menu mainMenu;
    try {
        UI::PointerCursor cursor;
        auto *texButton = new GX::Texture(gRootDir / "res/button.tex");
        auto sprBg = loadBackground();

        auto item = new UI::MenuItem("First Item", MenuItemOneActivate);
        item->setTexture(texButton);
        mainMenu.addItem(item);

        item = new UI::MenuItem("Item the Second", nullptr);
        item->setTexture(texButton);
        mainMenu.addItem(item);

        item = new UI::MenuItem("here's three!", nullptr);
        item->setEnabled(false)->setTexture(texButton);
        mainMenu.addItem(item);

        item = new UI::MenuItem("This item's text is way too long what the hell kind of menu item even is this supposed to be", nullptr);
        item->setTexture(texButton);
        mainMenu.addItem(item);

        int irX=0, irY=0;
        while(!bExit) {
            GX::frameBegin();
            sprBg->draw();

            PAD_ScanPads();
            u32 wpadButtonsDown = updateWpad(&irX, &irY);
            if(wpadButtonsDown & WPAD_BUTTON_HOME) {
                printf("Home pressed!\r\n");
                exit(0);
            }
            if(wpadButtonsDown & WPAD_BUTTON_DOWN) mainMenu.move( 1);
            if(wpadButtonsDown & WPAD_BUTTON_UP)   mainMenu.move(-1);
            if(wpadButtonsDown & WPAD_BUTTON_A)    mainMenu.select();

            char msg[256];
            sprintf(msg, "cursor %d %d", irX, irY);
            gSystemFont
                ->setSize(16)
                ->setPos(0, 16)
                ->setColor({255, 255, 255, 255})
                ->drawString(msg);

            mainMenu.draw();

            //draw cursor above everything else.
            cursor.setPos(irX, irY);
            cursor.draw();

            GX::frameEnd();
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
    if(bShutDown) STM_ShutdownToStandby();
    if(bReboot) STM_RebootSystem();
	return 0;
}
