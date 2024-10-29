#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include "Gx.h"
};

//g0, g1, g2, g3: game ID
//c0, c1: company ID
//d: disc number
//v: version
#define MAKE_DISC_ID(g0, g1, g2, g3, c0, c1, d, v) ( \
    ((u64)g0 << 56) | \
    ((u64)g1 << 48) | \
    ((u64)g2 << 40) | \
    ((u64)g3 << 32) | \
    ((u64)c0 << 24) | \
    ((u64)c1 << 16) | \
    ((u64)d  <<  8) | \
    (u64)v)

namespace Sys {
    /**
     * @brief Handles disc drive I/O.
     */
    class DiscDrive {
        public:
            //US versions
            static constexpr const u64 DISC_ID_SFA_U0 = MAKE_DISC_ID('G', 'S', 'A', 'E', '0', '1', 0, 0);
            static constexpr const u64 DISC_ID_SFA_U1 = MAKE_DISC_ID('G', 'S', 'A', 'E', '0', '1', 0, 1);
            //JP versions
            static constexpr const u64 DISC_ID_SFA_J0 = MAKE_DISC_ID('G', 'S', 'A', 'J', '0', '1', 0, 0);
            static constexpr const u64 DISC_ID_SFA_J1 = MAKE_DISC_ID('G', 'S', 'A', 'J', '0', '1', 0, 1);
            //EU versions
            static constexpr const u64 DISC_ID_SFA_E0 = MAKE_DISC_ID('G', 'S', 'A', 'P', '0', '1', 0, 0);
            static constexpr const u64 DISC_ID_SFA_E1 = MAKE_DISC_ID('G', 'S', 'A', 'P', '0', '1', 0, 1);
            DiscDrive();
            int getStatus();
            int getDiscId(u64 &id);
    };
};
