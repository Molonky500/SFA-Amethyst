#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};
#include "GcIso/Iso.h"

//raw DVD register access because libogc doesn't work.
typedef struct {
    volatile u32 SR;      //status register
    volatile u32 CVR;     //cover register (status2)
    volatile u32 CMDBUF0; //command buffer 0
    volatile u32 CMDBUF1; //command buffer 1
    volatile u32 CMDBUF2; //command buffer 2
    volatile u32 MAR;     //DMA memory address register
    volatile u32 LENGTH;  //DMA transfer length register
    volatile u32 CR;      //control register
    volatile u32 IMMBUF;  //immediate data buffer
    volatile u32 CFG;     //configuration register
} DI_Struct;
extern DI_Struct *DI;

#define DISR_BRKINT (1 << 6)
#define DISR_BRKINTMASK (1 << 5)
#define DISR_TCINT (1 << 4)
#define DISR_TCINTMASK (1 << 3)
#define DISR_DEINT (1 << 2)
#define DISR_DEINTMASK (1 << 1)
#define DISR_BRK (1 << 0)

#define DICVR_CVRINT (1 << 2) //Cover Interrupt Status
#define DICVR_CVRINTMASK (1 << 1) //Cover Interrupt Mask
#define DICVR_CVR (1 << 0) //0:cover closed, 1:open

#define DICR_RW (1 << 2)
#define DICR_DMA (1 << 1)
#define DICR_TSTART (1 << 0)

#define DICMD_INQUIRY    0x12000000
#define DICMD_READ       0xA8000000
#define DICMD_IDENTIFY   0xA8000040 //read disc ID/init drive
#define DICMD_SEEK       0xAB000000
#define DICMD_READ_DVD   0xD0000000 //for video DVDs/DVDRs
#define DICMD_GET_ERROR  0xE0000000
#define DICMD_STOP_MOTOR 0xE3000000

//DVD error codes high byte
#define DVD_ERR_LID_OPEN     0x01000000
#define DVD_ERR_DISC_CHANGED 0x02000000
#define DVD_ERR_NO_DISC      0x03000000
#define DVD_ERR_MOTOR_OFF    0x04000000
#define DVD_ERR_NOT_INIT     0x05000000 //disc ID not read

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

namespace Sys { namespace Dvd {
    constexpr const int SECTOR_SIZE = 2048; //bytes per sector
    //US versions
    constexpr const u64 DISC_ID_SFA_U0 = MAKE_DISC_ID('G', 'S', 'A', 'E', '0', '1', 0, 0);
    constexpr const u64 DISC_ID_SFA_U1 = MAKE_DISC_ID('G', 'S', 'A', 'E', '0', '1', 0, 1);
    //JP versions
    constexpr const u64 DISC_ID_SFA_J0 = MAKE_DISC_ID('G', 'S', 'A', 'J', '0', '1', 0, 0);
    constexpr const u64 DISC_ID_SFA_J1 = MAKE_DISC_ID('G', 'S', 'A', 'J', '0', '1', 0, 1);
    //EU versions
    constexpr const u64 DISC_ID_SFA_E0 = MAKE_DISC_ID('G', 'S', 'A', 'P', '0', '1', 0, 0);
    constexpr const u64 DISC_ID_SFA_E1 = MAKE_DISC_ID('G', 'S', 'A', 'P', '0', '1', 0, 1);

    /**
     * @brief Handles disc drive I/O.
     * @details Creates a virtual device at "dvdraw:" which
     *  can be opened with fopen() to read the DVD directly.
     *  Alignment and size requirements are handled transparently.
     *  For accessing files on the disc, use GcIso.
     */
    class Device {
        public:
            Device();
            ~Device();
            int getStatus();
            u32 getError();
            int getDiscId(u64 &id);
            int read(void *dst, uint32_t size, uint32_t offset);

        protected:
            std::string cwd;
            GcIso::Iso *iso;

            void _reset();
            void _writeCmd(u32 cmd0, u32 cmd1, u32 cmd2);
            void _doXfer(u32 cr);
            void _identify();
            int _read(void *dst, uint32_t size, uint32_t offset, bool isDvdVideo);
    };
}};
