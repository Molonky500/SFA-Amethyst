#include "main.h"
#include "revolution/gx/GXEnum.h"

static u32 prevState[0x35];

void beginDebugRender() {
    //back up prevous state
    gxGetVtxDescrs(prevState);

    //copied from various functions.
    //possibly some of these calls aren't needed.
    //XXX some of these functions have wrong names.
    gxSetZMode_(1, GX_LESS, 0); //enable depth test, don't update depth buffer
    gxResetVtxDescr();
    gxSetVtxDescr(GX_VA_POS,  GX_DIRECT);
    gxSetVtxDescr(GX_VA_CLR0, GX_DIRECT);
    gxSetMtx43((Mtx*)getCameraMtxs(), 0);
    gxSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetTevAlphaIn(7, GX_CA_APREV, GX_CA_APREV, GX_CA_ZERO, GX_CA_APREV);
    gxSetBackfaceCulling(0);
    gxSetKSel(0, 0x1c);
    gxSetTevKsel(0, 0xc);
    gxSetRasTref(0, 0xff, 0xff, 4);
    gxResetIndCmd(0);
    gxSetTexColorEnv0(0, 0xf, 0xf, 0xf, 0xe);
    _gxSetTexColorEnv(0, 7, 7, 7, 6);
    gxSetTexColorEnv1(0, 0, 0);
    _gxSetTexColorEnv0(0, 0, 0, 0, 1, 0);
    _gxSetTexColorEnv1(0, 0, 0, 0, 1, 0);
    gxSetColorCtrl(0, 0, 0, 1, 0, 0, 2);
    gxSetColorCtrl(2, 0, 0, 1, 0, 0, 2);
    gxSetNumColors(1);
    gxTextureFn_8025b6f0(0);
    gxSetNumTextures(0);
    gxTextureFn_8025c2a0(1);
}

void endDebugRender() {
    //XXX reset other state
    //gxSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_DSTCLR, GX_LO_NOOP);
    gxSetVtxDescrs(prevState);
    gxSetBackfaceCulling(1);
}

void begin2D(Color4b *color) {
    /** Set up to render 2D graphics.
     *  @param color If not NULL, color to use. Otherwise, keep previous color.
     */
    gxResetVtxDescr();
    gxSetVtxDescr(GX_VA_PNMTXIDX,GX_DIRECT);
    gxSetVtxDescr(GX_VA_POS,GX_DIRECT);
    gxSetBackfaceCulling(0);
    gxSetProjection((Mtx*)&hudMatrix,TRUE);
    gxSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_NOOP);
    if(color) GXSetTevKColor_(0,color);

    //copied from game code. not sure what most of this does.
    GXSetTevKAlphaSel(0,0x1c);
    gxSetTevKsel(0,0xc);
    GXSetTevOrder(0,0xff,0xff,4);
    gxResetIndCmd(0);
    GXSetTevColorIn(0,0xf,0xf,0xf,0xe);
    _gxSetTexColorEnv(0,7,7,7,6);
    gxSetTexColorEnv1(0,0,0);
    GXSetTevColorOp(0,0,0,0,1,0);
    GXSetTevAlphaOp(0,0,0,0,1,0);
    gxSetColorCtrl(0,0,0,1,0,0,2);
    gxSetColorCtrl(2,0,0,1,0,0,2);
    GXSetNumChans(1);
    GXSetNumIndStages(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
}

void write2Dvtx(float x, float y) {
    static volatile u8  *fifoU8  = (volatile u8*) GX_FIFO_BASE;
    static volatile s16 *fifoS16 = (volatile s16*)GX_FIFO_BASE;
    *fifoU8  = 0x3C; //PNMTXIDX
    *fifoS16 = x * hudScale;
    *fifoS16 = y * hudScale;
    *fifoS16 = -8;
}

void draw2Dbox(float x, float y, float w, float h, const Color4b *color) {
    /** Draw a filled 2D box.
     *  @param x X coordinate on screen.
     *  @param y Y coordinate on screen.
     *  @param w Width in pixels.
     *  @param h Height in pixels.
     *  @param color If not NULL, color to use. Otherwise, keep previous color.
     */
    if(color) GXSetTevKColor_(0,(Color4b*)color);
    if(w<0.1 || h<0.1) return;
    GXBegin(GX_QUADS, 1, 4);
    write2Dvtx(x,   y);
    write2Dvtx(x+w, y);
    write2Dvtx(x+w, y+h);
    write2Dvtx(x,   y+h);
}

void _drawVel(vec3f base, vec3f tip, Color4b color) {
    //draw a thin pyramid with its (not drawn) base centred around "base"
    //and its tip at "tip".
    //similar to drawArrow() but different params, no base
    GXSetTevKColor_(0, &color);

    vec3f TL  = {-1.0f + base.x, -1.0f + base.y, 0.0f + base.z};
    vec3f TR  = { 1.0f + base.x, -1.0f + base.y, 0.0f + base.z};
    vec3f BR  = { 1.0f + base.x,  1.0f + base.y, 0.0f + base.z};
    vec3f BL  = {-1.0f + base.x,  1.0f + base.y, 0.0f + base.z};

    GXBegin(GX_TRIANGLEFAN, 2, 6);
    drawSolidVtx(tip, &color);
    //top
    drawSolidVtx(TL,  &color);
    drawSolidVtx(TR,  &color);
    //right
    drawSolidVtx(BR,  &color);
    //bottom
    drawSolidVtx(BL,  &color);
    //left
    drawSolidVtx(TL,  &color);
}

void _renderPlayerVel() {
    ObjInstance *player = pPlayer;
    if(!player) return;

    Color4b color = {255, 0, 0, 128};
    vec3f pos = player->pos.pos;
    pos.y += 30; //from head, not bottom
    pos.x -= playerMapOffsetX;
    pos.z -= playerMapOffsetZ;

    //scale by 60 to show where you'll be in one second
    //because otherwise it's too small to see.
    vec3f tip = pos;
    tip.x += (player->vel.x * 60);
    tip.y += (player->vel.y * 60);
    tip.z += (player->vel.z * 60);
    _drawVel(pos, tip, color);

    //if there's a water current, draw it too
    if(player->catId != ObjCatId_Player) return;
    void *pState = pPlayer ? pPlayer->state : NULL;
    if(!pState) return;

    float waterDepth = *(float*)(pState + 0x838); //how deep we are in it
    if(waterDepth <= 0) return;
    vec3f vel = {0, 0, 0};
    vel.x = *(float*)(pState + 0x648); //water current, player-relative
    vel.z = *(float*)(pState + 0x64C);
    tip.x = pos.x + (vel.x * 60);
    tip.y = pos.y + (vel.y * 60);
    tip.z = pos.z + (vel.z * 60);
    color.r=0; color.g=128; color.b=255; color.a=128;
    _drawVel(pos, tip, color);
}

void _renderCurve(RomCurve *curve) {
    Color4b col = hsv2rgb(curve->type << 2, 255, 255, 255);
        vec3f pos = curve->def.pos;
        pos.x -= playerMapOffsetX;
        pos.z -= playerMapOffsetZ;
        vec3s rot = {curve->rotX << 8, curve->rotY << 8, curve->rotZ << 8};
        drawArrow(pos, rot, 10.0f, col);

    //draw the lines between them
    RomCurve *next[3];
    next[0] = RomCurve_getById(curve->idNext);
    next[1] = RomCurve_getById(curve->id24);
    next[2] = RomCurve_getById(curve->id28);
    for(int i=0; i<3; i++) {
        if(next[i]) {
            GXBegin(GX_LINES, 2, 2);
            drawSolidVtx(pos, &col);
            vec3f pos2 = next[i]->def.pos;
            pos2.x -= playerMapOffsetX;
            pos2.z -= playerMapOffsetZ;
            drawSolidVtx(pos2, &col);
            //XXX handle the actual curve part (bezier, etc)
        }
    }
}

void renderCurves() {
    RomCurve **curves = 0x803a17e8;
    int nCurves = *(int*)0x803dd478;
    debugPrintf("%d curves\n", nCurves);
    GXSetLineWidth(24, 0);
    GXSetPointSize(24, 0);
    for(int iCurve=0; iCurve<nCurves; iCurve++) {
        _renderCurve(curves[iCurve]);
    }
}

void renderObjsHook(u8 param) {
    //renderObjects(visible);
    WRITE8(0x803db658, param); //replaced call just does this
    if(debugRenderFlags & (DEBUGRENDER_HITBOXES | DEBUGRENDER_ATTACH_POINTS |
        DEBUGRENDER_FOCUS_POINTS | DEBUGRENDER_UNK_POINTS | DEBUGRENDER_PLAYER_VEL |
        DEBUGRENDER_CURVES)) {
        beginDebugRender();
        if(debugRenderFlags & DEBUGRENDER_HITBOXES) {
            for(int iObj=0; iObj < numLoadedObjs; iObj++) {
                _renderObjHitboxes(loadedObjects[iObj]);
            }
        }

        //this misses some objects
        /* for(int iObj=0; iObj < objRenderListLen; iObj++) {
            u32 idx = objRenderList[iObj] & 0x3FF; //no idea what the other bits are...
            if(loadedObjects[idx]) {
                _renderObjHitboxes(loadedObjects[idx]);
            }
        } */

        if(debugRenderFlags & DEBUGRENDER_PLAYER_VEL) _renderPlayerVel();
        if(debugRenderFlags & DEBUGRENDER_CURVES) renderCurves();
        endDebugRender();
    }
}

void renderHooksInit() {
    //hookBranch(0x8005c458, renderObjsHook, 1);
    hookBranch(0x8005c72c, renderObjsHook, 1);
}
