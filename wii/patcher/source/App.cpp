#include "main.h"
#include "MainMenu.h"

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
    this->sprCursor = nullptr;
    this->cursorOffsetX = -200; //XXX why is this needed?
    this->cursorOffsetY = -200;
    this->screenFadeOpacity = 0;
    this->gcStickDeadZone = 8;
}

App::~App() {
    if(this->systemFont) delete this->systemFont;
    if(this->sprBg) delete this->sprBg;
    if(this->mainMenu) delete this->mainMenu;
    if(this->sprCursor) delete this->sprCursor;
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
    this->sprCursor = new UI::PointerCursor();
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
    auto tex = this->loadTexture("bg");
    this->sprBg = new GX::Sprite(tex);

    u16 screenW, screenH, texW, texH;
    GX::getScreenSize(screenW, screenH);
    tex->getSize(&texW, &texH);
    this->sprBg->setPos((screenW/2)-(texW/2), (screenH/2)-(texH/2));
}


void App::_initMainMenu() {
    this->mainMenu = new UI::MainMenu();
    this->curMenu = this->mainMenu;
}

void App::_updateWpads() {
    WPADData *wpad;
    WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, 640 + 128, 480 + 128);
    WPAD_ScanPads();
    wpad = WPAD_Data(0);

    if(wpad->ir.smooth_valid) {
        this->cursorX = wpad->ir.sx + this->cursorOffsetX;
        this->cursorY = wpad->ir.sy + this->cursorOffsetY;
    }
    else if(wpad->ir.raw_valid) {
        this->cursorX = wpad->ir.ax + this->cursorOffsetX;
        this->cursorY = wpad->ir.ay + this->cursorOffsetY;
    }
}

void App::_updateGpads() {
    PAD_ScanPads();
    s32 stickX = PAD_StickX(0);
    s32 stickY = PAD_StickY(0);
    if(stickX < this->gcStickDeadZone && stickX > -this->gcStickDeadZone) stickX = 0;
    if(stickY < this->gcStickDeadZone && stickY > -this->gcStickDeadZone) stickY = 0;
    if(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z) {
        stickX *= 4;
        stickY *= 4;
    }
    this->cursorX += stickX / 4;
    this->cursorY -= stickY / 4;
}

void App::_clampCursor() {
    u16 screenW, screenH;
    GX::getScreenSize(screenW, screenH);
    if(this->cursorX < 0) this->cursorX = 0;
    if(this->cursorX >= screenW) this->cursorX = screenW-1;
    if(this->cursorY < 0) this->cursorY = 0;
    if(this->cursorY >= screenH) this->cursorY = screenH-1;
}

u32 App::_getButtonsDown() {
    u32 wb = WPAD_ButtonsDown(0);
    u32 gb = PAD_ButtonsDown(0);

    //map GC buttons to Wii buttons
    if(gb & PAD_BUTTON_A)     wb |= WPAD_BUTTON_A;
    if(gb & PAD_BUTTON_B)     wb |= WPAD_BUTTON_B;
    if(gb & PAD_BUTTON_UP)    wb |= WPAD_BUTTON_UP;
    if(gb & PAD_BUTTON_DOWN)  wb |= WPAD_BUTTON_DOWN;
    if(gb & PAD_BUTTON_LEFT)  wb |= WPAD_BUTTON_LEFT;
    if(gb & PAD_BUTTON_RIGHT) wb |= WPAD_BUTTON_RIGHT;
    if(gb & PAD_BUTTON_START) wb |= WPAD_BUTTON_HOME;
    if(gb & PAD_BUTTON_X)     wb |= WPAD_BUTTON_2;
    if(gb & PAD_BUTTON_Y)     wb |= WPAD_BUTTON_1;
    if(gb & PAD_TRIGGER_L)    wb |= WPAD_BUTTON_MINUS;
    if(gb & PAD_TRIGGER_R)    wb |= WPAD_BUTTON_PLUS;
    return wb;
}

void App::_handleControllers() {
    this->_updateWpads();
    this->_updateGpads();
    this->_clampCursor();
    this->sprCursor->setPos(this->cursorX, this->cursorY);

    u32 buttons = this->_getButtonsDown();
    if(buttons & WPAD_BUTTON_HOME) {
        printf("Home pressed!\r\n");
        _exitMode = App::ExitMode::QUIT;
        return;
    }
    this->curMenu->handlePointer(this->cursorX, this->cursorY);
    if(buttons & WPAD_BUTTON_DOWN) this->curMenu->move( 1);
    if(buttons & WPAD_BUTTON_UP)   this->curMenu->move(-1);
    if(buttons & WPAD_BUTTON_A)    this->curMenu->select();
}

void App::_drawScreenFadeOverlay() {
    if(this->screenFadeOpacity <= 0) return;

    u16 screenW, screenH;
    GX::getScreenSize(screenW, screenH);
    GX::Color color = {0x00, 0x00, 0x00, 0xFF * this->screenFadeOpacity};

    GX::gBlankTexture->select();
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
    GX::frameBegin();
    if(this->sprBg) this->sprBg->draw();

    char msg[256];
    sprintf(msg, "cursor %d %d", this->cursorX, this->cursorY);
    this->systemFont
        ->setSize(16)
        ->setPos(0, 16)
        ->setColor({255, 255, 255, 255})
        ->drawString(msg);

    this->mainMenu->draw();
    this->sprCursor->draw();
    this->_drawScreenFadeOverlay();
    GX::frameEnd();
    this->frameCount++;
}

void App::_fadeOut() {
    for(int i=0; i<30; i++) {
        this->screenFadeOpacity += 1.0f / 30.0f;
        if(this->screenFadeOpacity > 1.0f) this->screenFadeOpacity = 1.0f;
        this->_draw();
    }
    //ensure we're entirely faded out
    this->screenFadeOpacity = 1.0f;
    this->_draw();
}

App::ExitMode App::run() {
    this->_init();
    while(_exitMode == App::ExitMode::KEEP_GOING) {
        this->_handleControllers();
        this->_draw();
    }
    this->_fadeOut();
    return _exitMode;
}
