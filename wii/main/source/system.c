#include "main.h"

int main(int argc, char **argv);

void SYS_Init(void) {
    //called from crt0.S
    u32 level;
	_CPU_ISR_Disable(level);
    main(_argc, _argc_raw);
}
