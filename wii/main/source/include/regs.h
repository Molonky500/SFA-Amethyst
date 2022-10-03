#ifndef __REGS_H__
#define __REGS_H__

#define PI_INSTR        *(vu32*)0xCC003000
#define PI_INMTR        *(vu32*)0xCC003004
//these have NOT moved to CDxxxxxx
#define AR_DMA_MMADDR_H *(vu16*)0xCC005020
#define AR_DMA_MMADDR_L *(vu16*)0xCC005022
#define AR_DMA_ARADDR_H *(vu16*)0xCC005024
#define AR_DMA_ARADDR_L *(vu16*)0xCC005026
#define AR_DMA_CNT_H    *(vu16*)0xCC005028
#define AR_DMA_CNT_L    *(vu16*)0xCC00502A
#define AR_DMA_CNT_LEFT *(vu16*)0xCC00503A
#define AI_DMA_START_HI *(vu16*)0xCC005030
#define AI_DMA_START_LO *(vu16*)0xCC005032
#define AI_DMA_LENGTH   *(vu16*)0xCC005036

//regs.c
extern vu32* const _piReg;
extern vu16* const _memReg;
extern vu16* const _dspReg;
extern vu32* const _ipcReg;
extern vu32* const _exiReg;
extern vu32* const _aiReg;

#endif //__REGS_H__
