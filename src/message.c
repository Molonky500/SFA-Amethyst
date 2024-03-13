#include "main.h"

#define MSGBOX_PAD_X 20 //padding around the outside of the box
#define MSGBOX_PAD_Y 20
#define MSGBOX_TEXT_PAD_X 10 //padding around the inside of the box
#define MSGBOX_TEXT_PAD_Y 10

int showPopupMessage(const char *text, const char **options) {
    /** Pause the game and display a popup message.
     *  @param text The message text.
     *  @param options The options the player can choose. Ends with a NULL pointer.
     *  @return The index of the selected option, or -1 if player presses B.
     *  @note Does not return until the player chooses an option or presses B.
     */
    int curOption = 0, nOptions = 0;
    Color4b messageColor    = {.r=192, .g=192, .b=192, .a=255};
    Color4b selectedColor   = {.r=255, .g= 64, .b= 64, .a=255};
    Color4b unselectedColor = {.r=128, .g=128, .b=128, .a=255};
    int stickDelay = 0;
    u8 frame = 0;

    bool japanese = (curLanguage == LANG_JAPANESE);
    int height = (japanese ? LINE_HEIGHT_JAPANESE : LINE_HEIGHT) + 4;

    while(1) {
        checkReset();
        padUpdate();
        mmFreeTick(); //game calls this in such loops
        waitNextFrame();

        //draw the background image
        Color4b bgCol = {.r=128, .g=128, .b=128, .a=128};
        hudDrawColored(getLastRenderedFrame(),0,0,&bgCol,0x200,0);

        drawBox(MSGBOX_PAD_X, MSGBOX_PAD_Y,
            SCREEN_WIDTH - (MSGBOX_PAD_X*2),
            SCREEN_HEIGHT - (MSGBOX_PAD_Y*2),
            128, true);
        int x = MSGBOX_PAD_X + MSGBOX_TEXT_PAD_X;
        int y = MSGBOX_PAD_Y + MSGBOX_TEXT_PAD_Y;
        drawText(text, x, y, NULL, &y, TEXT_COLORED | TEXT_SHADOW, messageColor, 1.0);
        y += height;

        selectedColor = hsv2rgb(frame++, 255, 255, 255);
        for(nOptions=0; options[nOptions]; nOptions++) {
            drawColorText(options[nOptions], x, y,
                (nOptions==curOption) ? selectedColor : unselectedColor);
            y += height;
        }
        GXFlush_(true, 0); //actually show the message

        u16 buttons = getButtonsJustPressed(0);
        if(buttons & PAD_BUTTON_B) return -1;
        if(buttons & PAD_BUTTON_A) return curOption;

        if(stickDelay) stickDelay--;
        else {
            s8 stick = padGetStickY(0);
            if(stick < -28) {
                curOption -= 1;
                if(curOption < 0) curOption = nOptions - 1;
                stickDelay = 30;
            }
            if(stick > 28) {
                curOption += 1;
                if(curOption >= nOptions) curOption = 0;
                stickDelay = 30;
            }
        }
    }
}
