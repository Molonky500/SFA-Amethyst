#pragma once
extern "C" {
    #include <stdlib.h>
    #include <gccore.h>
    #include <sys/types.h>
    #include <math.h>
};

namespace GX {
    typedef struct {
        union {
            struct { u8 r; u8 g; u8 b; u8 a; };
            u32 value;
        };
    } Color;

    //vertex format we use
    typedef struct {
        s16 x, y;
        Color c;
        s16 s, t;
    } AppVtx;

    int init();
    void frameBegin();
    void frameEnd();
    void getScreenSize(u16 &width, u16 &height);
    Color hsv2rgb(u8 h, u8 s, u8 v, u8 a);
};
