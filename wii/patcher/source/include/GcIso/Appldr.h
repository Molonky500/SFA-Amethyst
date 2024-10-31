#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

namespace GcIso {
    class Appldr {
        public:
            typedef struct {
                char buildDate[0x10];
                u32 entryPoint;
                u32 dataSize;
                u32 trailerSize;
                u32 _pad1C;
            } Header;

            Header header;

            Appldr(Header header) {
                this->header = header;
            }

            u32 getTotalSize() {
                return this->header.dataSize +
                    this->header.trackSize +
                    sizeof(Header);
            }
    };
};
