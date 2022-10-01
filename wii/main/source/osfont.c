#include "main.h"

uint OSGetFontEncode_hook() {
    return 0; //English
    //return 1; //Japanese
}

char* OSGetFontWidth_hook(char* string, s32* width) {
    *width = 8;
    return &string[1];
}

char* OSGetFontTexel_hook(char* string, void* image, s32 pos,
s32 stride, s32* width) {
    // *width = 8;
    return &string[1];
}
