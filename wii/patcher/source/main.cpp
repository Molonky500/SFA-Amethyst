#include "main.h"

App *gApp;

int main(int argc, char **argv) {
    try {
        gApp = new App(argc, argv);
        switch(gApp->run()) {
            case App::ExitMode::QUIT: return 0;
            case App::ExitMode::REBOOT:
                STM_RebootSystem();
                break;
            case App::ExitMode::POWEROFF:
            default:
                STM_ShutdownToStandby();
                break;
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
    if(gApp) delete gApp;
    return 0;
}
