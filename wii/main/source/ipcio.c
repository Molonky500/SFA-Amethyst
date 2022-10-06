#include "main.h"

u32 IPC_ReadReg(u32 reg) {
	return _ipcReg[reg];
}

void IPC_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg] = val;
}

void ACR_WriteReg(u32 reg,u32 val) {
	_ipcReg[reg>>2] = val;
}

void _ipcAckReply() {
    //clear IY1, IY2 (interrupt flags); set Y1
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x04)); //HW_IPC_PPCCTRL
    ACR_WriteReg(48,0x40000000); //XXX why? we're not even
        //supposed to be able to access this.
}

void _ipcSendEndOfReply() {
    //clear IY1, IY2; clear X2 (by writing 1)
    IPC_WriteReg(1,((IPC_ReadReg(1)&0x30)|0x08));
}
