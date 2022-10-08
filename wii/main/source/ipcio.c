#include "main.h"

vu32 IPC_ReadReg(u32 reg) {
	return _ipcReg[reg];
}

void IPC_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg] = val;
}

void ACR_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg>>2] = val;
}

void _ipcAckReply() {
    //set Y1 (clear "reply available" flag)
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x04)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000); //HW_IPC_ARMCTRL
        //XXX why? we're not even supposed to be able to
        //access this, and this bit isn't even defined?
        //I think this is a mistake, and it's meant to
        //write to HW_PPCIRQFLAG, though doing that just
        //makes it hang entirely.
}

void _ipcSendEndOfReply() {
    //clear X2 (by writing 1)
    //this just seems to be a "relaunch" flag
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x08));
}
