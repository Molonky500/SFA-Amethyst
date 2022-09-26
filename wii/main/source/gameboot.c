#include "main.h"

void bootGame() {
    exiPrintf("game thread SP=%08X\n", get_r1());

    //fatUnmount("sd");

    //register interrupt handlers
    /*static OSContext ctxDspAi;
    static OSContext ctxDspAram;
    static OSContext ctxDsp;
    static OSContext ctxAi;
    static OSContext ctxPiCp;
    static OSContext ctxPiPeToken;
    static OSContext ctxPiPeFinish;
    static OSContext ctxPiSi;
    static OSContext ctxPiDi;
    static OSContext ctxPiVi;

    IRQ_Request(IRQ_DSP_AI,     gameIrqHandler, &ctxDspAi);
    IRQ_Request(IRQ_DSP_ARAM,   gameIrqHandler, &ctxDspAram);
    IRQ_Request(IRQ_DSP_DSP,    gameIrqHandler, &ctxDsp);
    IRQ_Request(IRQ_AI,         gameIrqHandler, &ctxAi);
    IRQ_Request(IRQ_PI_CP,      gameIrqHandler, &ctxPiCp);
    IRQ_Request(IRQ_PI_PETOKEN, gameIrqHandler, &ctxPiPeToken);
    IRQ_Request(IRQ_PI_PEFINISH,gameIrqHandler, &ctxPiPeFinish);
    IRQ_Request(IRQ_PI_SI,      gameIrqHandler, &ctxPiSi);
    IRQ_Request(IRQ_PI_DI,      gameIrqHandler, &ctxPiDi);
    IRQ_Request(IRQ_PI_VI,      gameIrqHandler, &ctxPiVi);
    //_gameUnmaskInterrupts(
    __UnmaskIrq(
        IRQMASK(IRQ_DSP_AI) |
        IRQMASK(IRQ_DSP_ARAM) |
        IRQMASK(IRQ_DSP_DSP) |
        IRQMASK(IRQ_AI) |
        IRQMASK(IRQ_PI_CP) |
        IRQMASK(IRQ_PI_PETOKEN) |
        IRQMASK(IRQ_PI_PEFINISH) |
        IRQMASK(IRQ_PI_SI) |
        IRQMASK(IRQ_PI_DI) |
        IRQMASK(IRQ_PI_VI));

    (*(u8*)0x803dca49) = 0; //isInitialized = false
    (*(u8*)0x803dca3d) = 0; //gameState = GAME_STATE_INIT
    //SYS_SetArena1Lo((void*)0x803fa480);
    //SYS_SetArena1Hi((void*)0x817ea240);
    (*(u32*)0x80000030) = 0x803fa480;
    (*(u32*)0x80000034) = 0x817ea240;*/

    //protect ARAM area from accidental accesses
    //SYS_ProtectRange(SYS_PROTECTCHAN0,
    //    (void*)0x90000000, 0x01000000, SYS_PROTECTNONE);

    void (*game_init_hw)(void) = (void (*)())0x80003354;
    exiPuts("game init hw\n");
    switchToGame();
    game_init_hw();
    switchToOgc();

    void (*game_init_user)(void) = (void (*)())0x80246cd4;
    exiPuts("game init user\n");
    switchToGame();
    game_init_user();
    switchToOgc();

    //set_msr(get_msr() | 0x4000);
    //int (*gameInit)(int, char**) = (int(*)(int, char**))0x80020d8c;
    int (*gameMain)(int, char**) = (int(*)(int, char**))0x8002133c;
    exiPuts("game main\n");
    u32 irq = IRQ_Disable();
    switchToGame();
    gameMain(0, NULL);
    while(1);

    //tell the compiler we're not coming back from this one.
    __builtin_unreachable();
}
