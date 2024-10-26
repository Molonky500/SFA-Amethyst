#include "main.h"
#define DEFAULT_FIFO_SIZE	(256*1024)

static void *frameBuffer[2] = { NULL, NULL};
static u32 curFbIdx = 0;
GXRModeObj *rmode;
f32 yscale;
u32 xfbHeight;
Mtx gMtxView;
Mtx44 gMtxPerspective;
Mtx44 gMtxProjOrtho;
GXColor bgColor = {0x00, 0x00, 0x00, 0xFF};

int GX::init() {
	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[curFbIdx]);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;
	gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);

	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

	GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern,
        GX_TRUE, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,
        ((rmode->viHeight == 2*rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(frameBuffer[curFbIdx], GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

    GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);

    /* This is needed so that we can specify the texture coordinates using
     * integers, where one unit in coordinate space is equivalent to a texel.
     */
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 1, 1);

    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
    GX_SetCopyClear(bgColor, GX_MAX_Z24);

	GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, 0,
                   GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

	// setup our camera at the origin
	// looking down the -z axis with y up
	/* guVector
        cam  = {0.0F, 0.0F,  0.0F},
        up   = {0.0F, 1.0F,  0.0F},
        look = {0.0F, 0.0F, -1.0F};
	guLookAt(gMtxView, &cam, &up, &look);

	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guPerspective(gMtxPerspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(gMtxPerspective, GX_PERSPECTIVE); */

    // matrix, t, b, l, r, n, f
    guOrtho(gMtxProjOrtho, 0, rmode->efbHeight, 0, rmode->fbWidth, 0, 1);
	return 0;
}

void GX::frameBegin() {
    GX_LoadProjectionMtx(gMtxProjOrtho, GX_ORTHOGRAPHIC);
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
}

void GX::frameEnd() {
    GX_DrawDone();

    curFbIdx ^= 1;
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(frameBuffer[curFbIdx],GX_TRUE);

    VIDEO_SetNextFramebuffer(frameBuffer[curFbIdx]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void GX::getScreenSize(u16 &width, u16 &height) {
    width  = rmode->fbWidth;
    height = rmode->efbHeight;
}

/** Convert HSV color to RGB.
 *  @param h Hue (0..255 = 0..359 degrees)
 *  @param s Saturation (0..255 = 0..100%)
 *  @param v Value (0..255 = 0..100%)
 *  @param a Alpha (0..255 = 0..100%)
 *  @return a GX::Color with the corresponding RGBA values.
 */
GX::Color GX::hsv2rgb(u8 h, u8 s, u8 v, u8 a) {
    float sf = s / 255.0;
    float vf = v / 255.0;
    float c = vf * sf;
    int h2 = h / 43; //h / 60 degrees
    float n = (fmod((h / 43.0), 2) - 1);
    if(n < 0) n = -n;
    float x = c * (1.0 - (float)n);
    float r=0, g=0, b=0;
    switch(h2) {
        case 0: r=c; g=x; b=0; break;
        case 1: r=x; g=c; b=0; break;
        case 2: r=0; g=c; b=x; break;
        case 3: r=0; g=x; b=c; break;
        case 4: r=x; g=0; b=c; break;
        case 5: r=c; g=0; b=x; break;
    }
    float m = vf - c;
    GX::Color res = {.r = (r+m)*255, .g = (g+m)*255, .b = (b+m)*255, .a = a};
    return res;
}
