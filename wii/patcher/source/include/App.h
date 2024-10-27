#pragma once
#include <filesystem>
extern "C" {
    #include <stdlib.h>
    #include <gccore.h>
    #include <sys/types.h>
    #include <math.h>
};
#include "Gx.h"
#include "Font.h"
#include "Texture.h"
#include "Sprite.h"
#include "Menu.h"
#include "PointerCursor.h"

/**
 * @brief The main application logic.
 */
class App {
    public:
        typedef enum {
            KEEP_GOING = -1, //don't exit yet (not valid return code)
            QUIT = 0, //return to loader/menu
            REBOOT, //reboot the system
            POWEROFF //power off the system
        } ExitMode;

        App(int argc, char **argv);
        ~App();
        ExitMode run();

        std::filesystem::path getRootDir() {
            return this->rootDir;
        }
        u32 getFrameCount() {
            return this->frameCount;
        }
        GX::Font* getSystemFont() {
            return this->systemFont;
        }

        GX::Texture* loadTexture(std::string name);

    protected:
        int argc;
        char **argv;
        std::filesystem::path rootDir;
        u32 frameCount;
        GX::Font *systemFont;
        GX::Sprite *sprBg;
        UI::Menu *curMenu, *mainMenu;
        UI::PointerCursor *sprCursor;
        int cursorX, cursorY;
        s32 cursorOffsetX, cursorOffsetY;
        float screenFadeOpacity;
        s8 gcStickDeadZone;

        void _init();
        void _initFilesystem();
        void _initGraphics();
        void _initBackground();
        void _initMainMenu();
        void _updateWpads();
        void _updateGpads();
        void _clampCursor();
        u32 _getButtonsDown();
        void _handleControllers();
        void _drawScreenFadeOverlay();
        void _draw();
        void _fadeOut();
};
