#include "main.h"

static App::ExitMode _exitMode = App::ExitMode::KEEP_GOING;

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;

static void MyStmHandler(u32 event) {
    switch(event) {
        case STM_EVENT_POWER:
            printf("STM power off\r\n");
            _exitMode = App::ExitMode::POWEROFF;
            break;

        case STM_EVENT_RESET:
            printf("STM reboot\r\n");
            _exitMode = App::ExitMode::REBOOT;
            break;

        default:
            printf("STM unknown event %d\r\n", event);
            return;
    }
}

App::App(int argc, char **argv) {
    this->argc = argc;
    this->argv = argv;
    this->frameCount = 0;
    this->systemFont = nullptr;
    this->sprBg = nullptr;
    this->mainMenu = nullptr;
    this->cursor = nullptr;
    this->screenFadeOpacity = 0;
}

App::~App() {
    if(this->systemFont) delete this->systemFont;
    if(this->sprBg) delete this->sprBg;
    if(this->mainMenu) delete this->mainMenu;
    if(this->cursor) delete this->cursor;
}

GX::Texture* App::loadTexture(std::string name) {
    //TODO: texture cache
    return new GX::Texture(this->rootDir / ("res/" + name + ".tex"));
}

void App::_init() {
    _ipcReg[0x64>>2]  = 0xFFFFFFFF; //AHBPROT: access everything
    _ipcReg[0xFC>>2] |= 0x00FF0020; //allow PPC access
    _ipcReg[0xDC>>2]  = 0xFFFFFFFF; //enable GPIOs
    _ipcReg[0xC4>>2] |= 0x00FF0020; //set directions

    SET_DISC_LED(1);
    GX::init();

    initDebugPrint();
    fprintf(stderr, " *** debug print online ***\r\n");
    this->systemFont = new GX::Font();

    WPAD_Init();
    PAD_Init();
    STM_RegisterEventHandler(MyStmHandler);

    this->_initFilesystem();
    this->_initGraphics();
    this->_initMainMenu();

    printf("Startup OK\r\n");
    SET_DISC_LED(0);
}

void App::_initFilesystem() {
    if(!fatInitDefault()) {
        throw new std::runtime_error("FAT init failed");
    }

    //argv is null if we were booted directly into Dolphin
    if(this->argv) this->rootDir = this->argv[0];
    else this->rootDir = "sd:/apps/patcher/";
}

void App::_initGraphics() {
    this->cursor = new UI::PointerCursor();
    try {
        this->_initBackground();
    }
    catch(std::exception &ex) {
        fprintf(stderr, "Failed to load background: %s\r\n", ex.what());
        if(this->sprBg) delete this->sprBg;
        this->sprBg = nullptr;
    }
}

void App::_initBackground() {
    auto tex = new GX::Texture(this->rootDir / "res/test.tex");
    this->sprBg = new GX::Sprite(tex);

    u16 screenW, screenH, texW, texH;
    GX::getScreenSize(screenW, screenH);
    tex->getSize(&texW, &texH);
    this->sprBg->setPos((screenW/2)-(texW/2), (screenH/2)-(texH/2));
}

static void MenuItemOneActivate(UI::MenuItem *item) {
    printf("This is item one\r\n");
}

void App::_initMainMenu() {
    this->mainMenu = new UI::Menu();

    auto *texButton = new GX::Texture(this->rootDir / "res/button.tex");

    auto item = new UI::MenuItem("First Item", MenuItemOneActivate);
    item->setTexture(texButton);
    this->mainMenu->addItem(item);

    item = new UI::MenuItem("Item the Second", nullptr);
    item->setTexture(texButton);
    this->mainMenu->addItem(item);

    item = new UI::MenuItem("here's three!", nullptr);
    item->setEnabled(false)->setTexture(texButton);
    this->mainMenu->addItem(item);

    item = new UI::MenuItem("This item's text is way too long what the hell kind of menu item even is this supposed to be", nullptr);
    item->setTexture(texButton);
    this->mainMenu->addItem(item);
}

u32 App::_updateWpad(int &outIrX, int &outIrY) {
    WPADData *wpad;
    WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, 640 + 128, 480 + 128);
    WPAD_ScanPads();
    wpad = WPAD_Data(0);

    if(wpad->ir.smooth_valid) {
        outIrX = wpad->ir.sx;
        outIrY = wpad->ir.sy;
    }
    else if(wpad->ir.raw_valid) {
        outIrX = wpad->ir.ax;
        outIrY = wpad->ir.ay;
    }

    return WPAD_ButtonsDown(0);
}

void App::_handleControllers() {
    PAD_ScanPads();
    u32 wpadButtonsDown = this->_updateWpad(this->cursorX, this->cursorY);
    this->cursor->setPos(this->cursorX, this->cursorY);

    if(wpadButtonsDown & WPAD_BUTTON_HOME) {
        printf("Home pressed!\r\n");
        _exitMode = App::ExitMode::QUIT;
        return;
    }
    if(wpadButtonsDown & WPAD_BUTTON_DOWN) this->mainMenu->move( 1);
    if(wpadButtonsDown & WPAD_BUTTON_UP)   this->mainMenu->move(-1);
    if(wpadButtonsDown & WPAD_BUTTON_A)    this->mainMenu->select();

    char msg[256];
    sprintf(msg, "cursor %d %d", this->cursorX, this->cursorY);
    this->systemFont
        ->setSize(16)
        ->setPos(0, 16)
        ->setColor({255, 255, 255, 255})
        ->drawString(msg);

}

void App::_drawScreenFadeOverlay() {
    if(this->screenFadeOpacity <= 0) return;
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    u16 screenW, screenH;
    GX::getScreenSize(screenW, screenH);
    GX::Color color = {0x00, 0x00, 0x00, 0xFF * screenFadeOpacity};

    //since the quad is black we don't care about what
    //texture is mapped, but we still need to supply
    //texture coords.
    //XXX this doesn't work, it uses whatever texture was
    //loaded before, and if that has a transparent pixel at
    //0,0 it doesn't do anything here.
    //not sure how to disable texturing, maybe should make up
    //a blank texture?
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    //top left
    GX_Position2s16(0, 0);
    GX_Color1u32(color.value);
    GX_TexCoord2s16(0, 0);

    //top right
    GX_Position2s16(screenW, 0);
    GX_Color1u32(color.value);
    GX_TexCoord2s16(0, 0);

    //bottom right
    GX_Position2s16(screenW, screenH);
    GX_Color1u32(color.value);
    GX_TexCoord2s16(0, 0);

    //bottom left
    GX_Position2s16(0, screenH);
    GX_Color1u32(color.value);
    GX_TexCoord2s16(0, 0);

    GX_End();
}

void App::_draw() {
    if(this->sprBg) this->sprBg->draw();
    this->mainMenu->draw();
    this->cursor->draw();
    this->_drawScreenFadeOverlay();
}

App::ExitMode App::run() {
    this->_init();
    while(_exitMode == App::ExitMode::KEEP_GOING) {
        GX::frameBegin();
        this->_handleControllers();
        this->_draw();
        GX::frameEnd();
        this->frameCount++;
    }
    //when exiting, fade out
    for(int i=0; i<60; i++) {
        this->screenFadeOpacity += 1.0f / 60.0f;
        GX::frameBegin();
        this->_draw();
        GX::frameEnd();
        this->frameCount++;
    }
    return _exitMode;
}
