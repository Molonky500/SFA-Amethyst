#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

namespace GcIso {
    class Bi2Bin {
        public:
            typedef struct {
                u32 debugMonitorSize;
                u32 simMemSize;
                u32 argOffs;
                u32 debugFlag;
                u32 trackLoc;
                u32 trackSize;
                u32 countryCode;
                u32 numDiscs;
                u32 supportLfn;
                u32 padSpec;
                u32 dolSizeLimit;
                u8 _pad1C[0x2000 - 0x1C];
            } Struct;
    };
};
