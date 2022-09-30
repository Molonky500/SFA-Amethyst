#include "main.h"

//Hollywood/Starlet (ARM) and Broadway (PPC) regs are the same,
//except a few bits.
//the high 2 bits are set on PPC.
//0x00800000 is a mask telling whether access perms are ARM/PPC.
//on PPC side it's forced to zero.
//so CD0xxxxx on PPC == 0D8xxxxx on ARM.
//ACR is ARM Control Reg? 48 >> 2 = 0xC which gives HW_IPC_ARMCTRL.
//ACR_WriteReg(r,v) == IPC_WriteReg(r>>2,v)
//no idea why write to that because PPC isn't supposed to have
//any access to it.

vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD000000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;
