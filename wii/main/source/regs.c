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
//any access to it, and the bit we write isn't mapped.
//seems like a mistake, that it's meant to be r, not r>>2,
//though it works as-is...
//once we set AHBPROT we can access 0xCD8xxxxx to have access
//to regs we normally don't.

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
// vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;
