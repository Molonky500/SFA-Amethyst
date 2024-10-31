#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

namespace GcIso {
    class BootBin {
        public:
            static constexpr const u32 MAGIC = 0xC2339F3D;

            typedef struct {
                char gameCode[4];
                char makerCode[2];
                u8 discNo;
                u8 version;
                u8 audioStreaming;
                u8 streamBufSize;
                u8 _pad0A[0x12];
                u32 magic; //0x1C, must be == MAGIC
                char gameName[0x3E0];
                u32 debugMonitorOffs; //0x400
                u32 debugMonitorAddr;
                u8 _pad408[0x18];
                u32 mainDolOffs; //0x420
                u32 fstOffs;
                u32 fstSize;
                u32 maxFstSize;
                u32 fstAddr;
                u32 fileOffset;
                u32 fileLength;
                u32 _pad43C;
            } Struct;
    };
};
