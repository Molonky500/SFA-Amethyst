#include "main.h"
#define DEBUG_IRQ 0

vs32  irqHandlerDepth =  0;
vs32  curIrqHandler   = -1;
vu32  lastIrqCause    =  0;
vu32  lastIrqCause2   =  0;
vu32 *gameIrqHandlers = (vu32*)0x80003040;

static u32 const _irqPrio[] = {
    IM_PI_ERROR, IM_PI_DEBUG, IM_MEM, IM_PI_RSW,
    IM_PI_VI, (IM_PI_PETOKEN|IM_PI_PEFINISH), IM_PI_HSP,
    (IM_DSP_ARAM|IM_DSP_DSP|IM_AI|IM_EXI|IM_PI_SI|IM_PI_DI),
    IM_DSP_AI, IM_PI_CP, IM_PI_ACR, 0xffffffff
};

#if DEBUG_IRQ
static const char *causes[] = {
	"GP Runtime Error", "Reset Switch", "DVD", "Serial",
	"EXI", "AI", "DSP", "MEM",
	"VI", "PE", "PE Finish", "CP FIFO",
	"Debugger", "HSP", "Hollywood IRQs", "RSVD15",
	"Reset Switch State", "RSVD17", "RSVD18", "RSVD19",
	"RSVD20", "RSVD21", "RSVD22", "RSVD23",
	"RSVD24", "RSVD25", "RSVD26", "RSVD27",
	"RSVD28", "RSVD29", "RSVD30", "RSVD31"
};
#endif


void gameExtIrqHandler_hook(int excNo, OSContext *ctx) {
    //copied from libogc to handle Wii IRQs
    u32 i,icause,intmask,irq = 0;

	u32 rawCause = _piReg[0];
	u32 mask     = _piReg[1];
	u32 cause    = rawCause & ~0x10000;
	lastIrqCause = rawCause;

	if(!cause || !(cause&mask)) {
		//spuriousIrq++;
		#if DEBUG_IRQ
			if(cause) { //ignore reset button
				char msg[64];
				strcpy(msg, "SPURIOUS IRQ ........ CAUSE=........ CTX=........\n");
				putHex(&msg[13], irqNo);
				putHex(&msg[28], rawCause);
				putHex(&msg[41], (u32)ctx);
				exiPuts(msg);
				for(int i=0; i<31; i++) {
					if(rawCause & (1 << i)) {
						exiPuts(causes[i]);
						exiPuts("\n");
					}
				}
			}
		#endif
		OSLoadContext(ctx);
		return;
	}

	intmask = 0;
	if(cause&0x00000080) {		//Memory Interface
		icause = _memReg[15];
		if(icause&0x00000001) {
			intmask |= IRQMASK(IRQ_MEM0);
		}
		if(icause&0x00000002) {
			intmask |= IRQMASK(IRQ_MEM1);
		}
		if(icause&0x00000004) {
			intmask |= IRQMASK(IRQ_MEM2);
		}
		if(icause&0x00000008) {
			intmask |= IRQMASK(IRQ_MEM3);
		}
		if(icause&0x00000010) {
			intmask |= IRQMASK(IRQ_MEMADDRESS);
		}
	}
	if(cause&0x00000040) {		//DSP
		icause = _dspReg[5];
		if(icause&0x00000008){
			intmask |= IRQMASK(IRQ_DSP_AI);
		}
		if(icause&0x00000020){
			intmask |= IRQMASK(IRQ_DSP_ARAM);
		}
		if(icause&0x00000080){
			intmask |= IRQMASK(IRQ_DSP_DSP);
		}
	}
	if(cause&0x00000020) {		//Streaming
		icause = _aiReg[0];
		if(icause&0x00000008) {
			intmask |= IRQMASK(IRQ_AI);
		}
	}
	if(cause&0x00000010) {		//EXI
		//EXI 0
		icause = _exiReg[0];
		if(icause&0x00000002) {
			intmask |= IRQMASK(IRQ_EXI0_EXI);
		}
		if(icause&0x00000008) {
			intmask |= IRQMASK(IRQ_EXI0_TC);
		}
		if(icause&0x00000800) {
			intmask |= IRQMASK(IRQ_EXI0_EXT);
		}
		//EXI 1
		icause = _exiReg[5];
		if(icause&0x00000002) {
			intmask |= IRQMASK(IRQ_EXI1_EXI);
		}
		if(icause&0x00000008) {
			intmask |= IRQMASK(IRQ_EXI1_TC);
		}
		if(icause&0x00000800) {
			intmask |= IRQMASK(IRQ_EXI1_EXT);
		}
		//EXI 2
		icause = _exiReg[10];
		if(icause&0x00000002) {
			intmask |= IRQMASK(IRQ_EXI2_EXI);
		}
		if(icause&0x00000008) {
			intmask |= IRQMASK(IRQ_EXI2_TC);
		}
	}
	if(cause&0x00002000) {		//High Speed Port
		intmask |= IRQMASK(IRQ_PI_HSP);
	}
	if(cause&0x00001000) {		//External Debugger
		intmask |= IRQMASK(IRQ_PI_DEBUG);
	}
	if(cause&0x00000400) {		//Frame Ready (PE_FINISH)
		intmask |= IRQMASK(IRQ_PI_PEFINISH);
	}
	if(cause&0x00000200) {		//Token Assertion (PE_TOKEN)
		intmask |= IRQMASK(IRQ_PI_PETOKEN);
	}
	if(cause&0x00000100) {		//Video Interface
		intmask |= IRQMASK(IRQ_PI_VI);
	}
	if(cause&0x00000008) {		//Serial
		intmask |= IRQMASK(IRQ_PI_SI);
	}
	if(cause&0x00000004) {		//DVD
		intmask |= IRQMASK(IRQ_PI_DI);
	}
	if(cause&0x00000002) {		//Reset Switch
		intmask |= IRQMASK(IRQ_PI_RSW);
	}
	if(cause&0x00000800) {		//Command FIFO
		intmask |= IRQMASK(IRQ_PI_CP);
	}
	if(cause&0x00000001) {		//GP Runtime Error
		intmask |= IRQMASK(IRQ_PI_ERROR);
	}
	if(cause&0x00004000) { //IOS IPC
		intmask |= IRQMASK(IRQ_PI_ACR);
	}

	lastIrqCause2 = icause;
	mask = intmask & ~(prevIrqMask|currIrqMask);
	if(mask) {
		i=0;
		irq = 0;
		while(i<(sizeof(_irqPrio)/sizeof(u32))) {
			if((irq=(mask&_irqPrio[i]))) {
				irq = cntlzw(irq);
				break;
			}
			i++;
		}

        //from here to end of function is from the game, not libogc.
		//mostly the same though.
        if(gameIrqHandlers[irq]) {
            OSDisableScheduler();
            void (*handler)(int,OSContext*) =
                (void(*)(int,OSContext*))gameIrqHandlers[irq];

			irqHandlerDepth++;
			s32 prevIrq  = curIrqHandler;
			curIrqHandler = irq;
			handler(irq,ctx);
			curIrqHandler = prevIrq;
			irqHandlerDepth--;

            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(ctx);
        }
        else {
            char msg[64];
            strcpy(msg, "no handler IRQ ........\n");
            putHex(&msg[15], irq);
            exiPuts(msg);
        }
	}
    OSLoadContext(ctx);
}

static u32 __SetInterrupts(u32 iMask,u32 nMask) {
	u32 imask;
	u32 irq;

	irq = cntlzw(iMask);
	if(irq<=IRQ_MEMADDRESS) {
		imask = 0;
		if(!(nMask&IM_MEM0)) imask |= 0x0001;
		if(!(nMask&IM_MEM1)) imask |= 0x0002;
		if(!(nMask&IM_MEM2)) imask |= 0x0004;
		if(!(nMask&IM_MEM3)) imask |= 0x0008;
		if(!(nMask&IM_MEMADDRESS)) imask |= 0x0010;
		_memReg[14] = (u16)imask;
		return (iMask&~IM_MEM);
	}

	if(irq>=IRQ_DSP_AI && irq<=IRQ_DSP_DSP) {
		imask = _dspReg[5]&~0x1f8;
		if(!(nMask&IM_DSP_AI)) imask |= 0x0010;
		if(!(nMask&IM_DSP_ARAM)) imask |= 0x0040;
		if(!(nMask&IM_DSP_DSP)) imask |= 0x0100;
		_dspReg[5] = (u16)imask;
		return (iMask&~IM_DSP);
	}

	if(irq==IRQ_AI) {
		imask = _aiReg[0]&~0x2c;
		if(!(nMask&IM_AI)) imask |= 0x0004;
		_aiReg[0] = imask;
		return (iMask&~IM_AI);
	}
	if(irq>=IRQ_EXI0_EXI && irq<=IRQ_EXI0_EXT) {
		imask = _exiReg[0]&~0x2c0f;
		if(!(nMask&IM_EXI0_EXI)) imask |= 0x0001;
		if(!(nMask&IM_EXI0_TC)) imask |= 0x0004;
		if(!(nMask&IM_EXI0_EXT)) imask |= 0x0400;
		_exiReg[0] = imask;
		return (iMask&~IM_EXI0);
	}

	if(irq>=IRQ_EXI1_EXI && irq<=IRQ_EXI1_EXT) {
		imask = _exiReg[5]&~0x0c0f;
		if(!(nMask&IM_EXI1_EXI)) imask |= 0x0001;
		if(!(nMask&IM_EXI1_TC)) imask |= 0x0004;
		if(!(nMask&IM_EXI1_EXT)) imask |= 0x0400;
		_exiReg[5] = imask;
		return (iMask&~IM_EXI1);
	}

	if(irq>=IRQ_EXI2_EXI && irq<=IRQ_EXI2_TC) {
		imask = _exiReg[10]&~0x000f;
		if(!(nMask&IM_EXI2_EXI)) imask |= 0x0001;
		if(!(nMask&IM_EXI2_TC)) imask |= 0x0004;
		_exiReg[10] = imask;
		return (iMask&~IM_EXI2);
	}

#if defined(HW_DOL)
	if(irq>=IRQ_PI_CP && irq<=IRQ_PI_HSP) {
#elif defined(HW_RVL)
	if(irq>=IRQ_PI_CP && irq<=IRQ_PI_ACR) {
#endif
		imask = 0xf0;
		if(!(nMask&IM_PI_ERROR)) {
			imask |= 0x00000001;
		}
		if(!(nMask&IM_PI_RSW)) {
			imask |= 0x00000002;
		}
		if(!(nMask&IM_PI_DI)) {
			imask |= 0x00000004;
		}
		if(!(nMask&IM_PI_SI)) {
			imask |= 0x00000008;
		}
		if(!(nMask&IM_PI_VI)) {
			imask |= 0x00000100;
		}
		if(!(nMask&IM_PI_PETOKEN)) {
			imask |= 0x00000200;
		}
		if(!(nMask&IM_PI_PEFINISH)) {
			imask |= 0x00000400;
		}
		if(!(nMask&IM_PI_CP)) {
			imask |= 0x00000800;
		}
		if(!(nMask&IM_PI_DEBUG)) {
			imask |= 0x00001000;
		}
		if(!(nMask&IM_PI_HSP)) {
			imask |= 0x00002000;
		}
#if defined(HW_RVL)
		if(!(nMask&IM_PI_ACR)) {
			imask |= 0x00004000;
		}
#endif
		_piReg[1] = imask;
		return (iMask&~IM_PI);
	}
	return 0;
}

void __UnmaskIrq(u32 nMask) {
	u32 level;
	u32 mask;

	_CPU_ISR_Disable(level);
	mask = (nMask&(prevIrqMask|currIrqMask));
	nMask = (prevIrqMask&~nMask);
	prevIrqMask = nMask;
	while((mask=__SetInterrupts(mask,(nMask|currIrqMask)))!=0);
	_CPU_ISR_Restore(level);
}

void __MaskIrq(u32 nMask) {
	u32 level;
	u32 mask;

	_CPU_ISR_Disable(level);
	mask = (nMask&~(prevIrqMask|currIrqMask));
	nMask = (nMask|prevIrqMask);
	prevIrqMask = nMask;
	while((mask=__SetInterrupts(mask,(nMask|currIrqMask)))!=0);
	_CPU_ISR_Restore(level);
}

void __OSInterruptInit_hook() {
    __OSInterruptInit();
    //gameIrqHandlers[IRQ_PI_ACR] = acrIrq; //set ACR IRQ handler (IOS IPC)
}

void __OSMaskInterrupts_hook(u32 mask) {
    //exiPrintf("OSMaskInterrupts(%08X) from %08X\n", mask, RETURN_ADDRESS);
    //mask &= ~IM_PI_ACR; //keep that one on
    __MaskIrq(mask);
}

void __OSUnmaskInterrupts_hook(u32 mask) {
    __UnmaskIrq(mask);
    //exiPrintf("OSUnmaskInterrupts(%08X) from %08X\n", mask, RETURN_ADDRESS);
}

void _irqPiError(int irq, OSContext *ctx) {
    char msg[64];
    strcpy(msg, "PI ERROR ctx=........\n");
    putHex(&msg[13], (u32)ctx);
    exiPuts(msg);
    gameExceptionHook(1, ctx, 0, NULL);
}

u32 IRQ_Disable(void) {
	u32 level;
	_CPU_ISR_Disable(level);
	return level;
}

void IRQ_Restore(u32 level) {
	_CPU_ISR_Restore(level);
}

bool areInterruptsEnabled() {
	return get_msr() & 0x8000;
}
