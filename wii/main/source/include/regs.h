#define PI_INSTR        *(vu32*)0xcc003000
#define PI_INMTR        *(vu32*)0xcc003004
#define AR_DMA_MMADDR_H *(vu16*)0xcc005020
#define AR_DMA_MMADDR_L *(vu16*)0xcc005022
#define AR_DMA_ARADDR_H *(vu16*)0xcc005024
#define AR_DMA_ARADDR_L *(vu16*)0xcc005026
#define AR_DMA_CNT_H    *(vu16*)0xcc005028
#define AR_DMA_CNT_L    *(vu16*)0xcc00502a
#define AR_DMA_CNT_LEFT *(vu16*)0xCC00503a
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
