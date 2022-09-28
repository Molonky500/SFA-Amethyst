#include "main.h"

u32 *gameIrqHandlers = (u32*)0x80003040;

static u32 const _irqPrio[] = {
    IM_PI_ERROR,IM_PI_DEBUG,IM_MEM,IM_PI_RSW,
    IM_PI_VI,(IM_PI_PETOKEN|IM_PI_PEFINISH),
    IM_PI_HSP,
    (IM_DSP_ARAM|IM_DSP_DSP|IM_AI|IM_EXI|IM_PI_SI|IM_PI_DI),
    IM_DSP_AI,IM_PI_CP,
    IM_PI_ACR,
    0xffffffff
};

struct irq_handler_s {
	void *pHndl;
	void *pCtx;
};
extern struct irq_handler_s g_IRQHandler[32];


void gameExtIrqHandler_hook(int irqNo, OSContext *ctx) {
    //copied from libogc to handle Wii IRQs
    u32 i,icause,intmask,irq = 0;
	u32 cause,mask;

	cause = _piReg[0]&~0x10000;
	mask = _piReg[1];

	if(!cause || !(cause&mask)) {
		//spuriousIrq++;
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
	if(cause&0x00004000) {
		intmask |= IRQMASK(IRQ_PI_ACR);
	}

	mask = intmask&~(prevIrqMask|currIrqMask);
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

        //from here to end of function is from the game, not libogc
        static u32 *handlers = (u32*)0x80003040;
        if(irq == IRQ_PI_ACR) {
            void (*handler)(int,OSContext*) =
                (void(*)(int,OSContext*))handlers[irq];

            /*char msg[64];
            strcpy(msg, "IRQ_PI_ACR ........ ........\n");
            putHex(&msg[11], handler);
            putHex(&msg[20], g_IRQHandler[irq].pCtx);
            exiPuts(msg);*/

            handler(irq,g_IRQHandler[irq].pCtx);
        }
        else if(handlers[irq]) {
            switchToGame();
            OSDisableScheduler();
            void (*handler)(int,OSContext*) =
                (void(*)(int,OSContext*))handlers[irq];
            handler(irq,ctx);
            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(ctx);
            switchToOgc();
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

void __OSInterruptInit_hook() {
    __OSInterruptInit();
    gameIrqHandlers[IRQ_PI_ACR] = acrIrq; //set ACR IRQ handler (IOS IPC)
}

void* __OSSetInterruptHandler_hook(int irq, void *handler) {
    switchToOgc();
    void *r = (void*)IRQ_Request(irq, (raw_irq_handler_t)handler,
        OSGetCurrentThread());
    switchToGame();
    return r;
}

void* __OSGetInterruptHandler_hook(int irq) {
    switchToOgc();
    void *r = (void*)IRQ_GetHandler(irq);
    switchToGame();
    return r;
}

void __OSMaskInterrupts_hook(u32 mask) {
    switchToOgc();
    exiPrintf("OSMaskInterrupts(%08X) from %08X\n", mask, RETURN_ADDRESS);
    //mask &= ~IM_PI_ACR; //keep that one on
    __MaskIrq(mask);
    switchToGame();
}

void __OSUnmaskInterrupts_hook(u32 mask) {
    switchToOgc();
    __UnmaskIrq(mask);
    exiPrintf("OSUnmaskInterrupts(%08X) from %08X\n", mask, RETURN_ADDRESS);
    switchToGame();
}

void _irqPiError(int irq, OSContext *ctx) {
    char msg[64];
    strcpy(msg, "PI ERROR ctx=........\n");
    putHex(&msg[13], ctx);
    exiPuts(msg);
    gameExceptionHook(1, ctx, 0, NULL);
}
