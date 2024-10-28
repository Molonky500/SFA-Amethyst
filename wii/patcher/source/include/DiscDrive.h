#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include "Gx.h"
};

namespace Sys {
    /**
     * @brief Handles disc drive I/O.
     */
    class DiscDrive {
        public:
            static constexpr const u64 DISC_ID_SFA_U0 = 0x4753414530310000; //"GSAE01\0\0"
            DiscDrive();
            int getStatus();
            int getDiscId(u64 &id);
    };
};
