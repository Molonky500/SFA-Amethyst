#include "main.h"

vu16* const _viReg  = (u16*)0xCC002000;
vu32* const _piReg  = (u32*)0xCC003000;
vu16* const _memReg = (u16*)0xCC004000;
vu16* const _dspReg = (u16*)0xCC005000;
vu32* const _ipcReg = (u32*)0xCD800000;
vu32* const _exiReg = (u32*)0xCD006800;
vu32* const _aiReg  = (u32*)0xCD006C00;

uint32_t crc32b(const void *data_, uint32_t len) {
    const uint8_t *data = (const uint8_t*)data_;
    unsigned int crc, mask;

    crc = 0xFFFFFFFF;
    for(uint32_t i=0; i<len; i++) {
        crc = crc ^ data[i];
        for(uint32_t j = 0; j < 8; j++) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    SET_DISC_LED(1);
    int err = 0;
    err = appGxInit();
    if(err) return err;

    appFontInit();

    // button initialization
    WPAD_Init();
    PAD_Init();
    SET_DISC_LED(0);
    return 0;
}

static AppVtx vtxsTest[] = {
    //gcc insists the color be wrapped in THREE sets of braces
    //x, y, r, g, b, a, s, t
    {100,  10, {{{0xFF, 0x00, 0x00, 0xFF}}}, 0x0080, 0x0000}, //top
    { 10, 100, {{{0x00, 0xFF, 0x00, 0xFF}}}, 0x0000, 0x0100}, //left
    {200, 100, {{{0x00, 0x00, 0xFF, 0xFF}}}, 0x0100, 0x0100}, //right
};
static void drawTestTriangle() {
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    for(int i=0; i<3; i++) {
        AppVtx *vtx = &vtxsTest[i];
        GX_Position2s16(vtx->x, vtx->y);
        GX_Color4u8(vtx->c.r, vtx->c.g, vtx->c.b, vtx->c.a);
        GX_TexCoord2s16(vtx->s, vtx->t);
    }
    GX_End();
}

int main(int argc, char **argv) {
    _ipcReg[0x64>>2]  = 0xFFFFFFFF; //AHBPROT: access everything
    _ipcReg[0xFC>>2] |= 0x00FF0020; //allow PPC access
    _ipcReg[0xDC>>2]  = 0xFFFFFFFF; //enable GPIOs
    _ipcReg[0xC4>>2] |= 0x00FF0020; //set directions

    int err = init();
    if(err) {
        fprintf(stderr, "Startup failed\n");
        exit(1);
    }

    int tick = 0;
    while(1) {
        appGxFrameBegin();
        PAD_ScanPads();

		drawTestTriangle();

        fontSetSize(16);
        fontSetPos(20, 260);
        fontSetColor(hsv2rgb(tick, 128, 128, 255));
        fontDrawString("Howdy doody\r\nbeans!");

        appGxFrameEnd();
        tick++;
    }
	return 0;
}
