#include "main.h"
#include "MenuInstallFromDisc.h"

static void onInstall(UI::MenuItem *item) {
    //FILE *file = fopen("dvd:/OBJECTS.bin", "rb");
    FILE *file = fopen("dvdraw:", "rb");
    printf("opened file: %p\r\n", file);
    if(!file) return;

    u8 data[256];
    fseek(file, 1, SEEK_SET);
    int r = fread(data, 1, 256, file);
    printf("read data, %d bytes\r\n", r);

    for(int offs=0; offs<r; offs += 16) {
        printf("%04X:", offs);
        for(int c=0; c<16; c++) {
            printf("%s%02X", (c&3) ? " " : "  ", data[offs+c]);
        }
        printf("  ");
        for(int c=0; c<16; c++) {
            u8 ch = data[offs+c];
            if(ch < 0x20 || ch > 0x7E) ch = 0x2E;
            printf("%c", ch);
        }
        printf("\r\n");
    }

    fclose(file);
    printf("Done reading.\r\n");
}

static void onBack(UI::MenuItem *item) {
    gApp->exitMenu();
}

void UI::MenuInstallFromDisc::draw() {
    this->sprStatusBg->draw();

    char msgBuf[256];
    std::string msg = "...";
    this->isDiscReady = false;

    #if 0
    int stat = gApp->dvd->getStatus();
    if     (stat & DVD_INIT) msg = "Initializing...";
    else if(stat & DVD_NO_DISC) msg = "Not inserted";
    else if(stat & DVD_IOS_ERROR) msg = "IOS error";
    else if(stat & DVD_READY) {
        u64 discId = 0;
        int err = gApp->dvd->getDiscId(discId);
        //printf("Disc ID 0x%llX, err %d\r\n", discId, err);
        if(err == -EAGAIN) msg = "Reading...";
        else if(err == 0) {
            switch(discId) {
                case Sys::Dvd::DISC_ID_SFA_U0:
                    msg = "OK (US V1.0)";
                    this->isDiscReady = true;
                    break;
                case Sys::Dvd::DISC_ID_SFA_U1:
                    msg = "Unsupported version (US V1.1)";
                    break;
                case Sys::Dvd::DISC_ID_SFA_J0:
                    msg = "Unsupported version (JP V1.0)";
                    break;
                case Sys::Dvd::DISC_ID_SFA_J1:
                    msg = "Unsupported version (JP V1.1)";
                    break;
                case Sys::Dvd::DISC_ID_SFA_E0:
                    msg = "Unsupported version (EU V1.0)";
                    break;
                case Sys::Dvd::DISC_ID_SFA_E1:
                    msg = "Unsupported version (EU V1.1)";
                    break;
                default:
                    sprintf(msgBuf, "Wrong game (%c%c%c%c%c%c)",
                        MAX(' ', (char)((discId >> 56) & 0xFF)),
                        MAX(' ', (char)((discId >> 48) & 0xFF)),
                        MAX(' ', (char)((discId >> 40) & 0xFF)),
                        MAX(' ', (char)((discId >> 32) & 0xFF)),
                        MAX(' ', (char)((discId >> 24) & 0xFF)),
                        MAX(' ', (char)((discId >> 16) & 0xFF)));
                    msg = msgBuf;
            }
        }
        else {
            sprintf(msgBuf, "Error %d", abs(err));
            msg = msgBuf;
        }
    }
    else msg = "Unknown state";
    #endif

    auto font = gApp->getSystemFont();
    font->setSize(30);
    font->setPos(20, 80)->setColor({0xFF, 0xFF, 0xFF, 0xFF})->drawString(
        ("Disc: " + msg).c_str());

    //this->itemInstall->setEnabled(this->isDiscReady);
    UI::Menu::draw();
}

UI::MenuInstallFromDisc::MenuInstallFromDisc(): UI::MenuInstallFromDisc::Menu() {
    this->isDiscReady = false;
    this->sprStatusBg = new GX::Sprite(gApp->loadTexture("button"));
    this->sprStatusBg->setPos(20, 40);
    this->posY = 300;
    this->itemInstall = new UI::MenuItem("Install", onInstall);
    this->addItem(this->itemInstall);
    this->addItem(new UI::MenuItem("Back", onBack));
}
