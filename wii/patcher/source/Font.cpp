#include "main.h"
extern "C" {

GX::Font::Font() {
    if(SYS_GetFontEncoding() == 0) {
        this->fontdata = (sys_fontheader*)memalign(32, SYS_FONTSIZE_ANSI);
    } else {
        this->fontdata = (sys_fontheader*)memalign(32, SYS_FONTSIZE_SJIS);
    }

    SYS_InitFont(fontdata);
    //this->selectTexture();
    this->size = this->fontdata->cell_height;
    this->posX = 0;
    this->posY = 0;
    this->color = {0xFF, 0xFF, 0xFF, 0xFF};
}

void GX::Font::selectTexture() {
    void *texels = (void *)this->fontdata + this->fontdata->sheet_image;
    GX_InitTexObj(&this->fonttex, texels,
        this->fontdata->sheet_width,
        this->fontdata->sheet_height,
        this->fontdata->sheet_format,
        GX_CLAMP, GX_CLAMP, GX_FALSE);

    GX_InitTexObjLOD(&this->fonttex, GX_LINEAR, GX_LINEAR, 0, 0, 0,
        GX_TRUE, GX_TRUE, GX_ANISO_1);
    GX_LoadTexObj(&this->fonttex, GX_TEXMAP0);

    u32 texture_size = GX_GetTexBufferSize(
        this->fontdata->sheet_width,
        this->fontdata->sheet_height,
        this->fontdata->sheet_format,
        GX_FALSE, 0);
    DCStoreRange(texels, texture_size);
    GX_InvalidateTexAll();
}

void GX::Font::drawCell(s16 x1, s16 y1, s16 s1, s16 t1) {
    u32 c = this->color.value;
    s16 x2 = x1 + this->fontdata->cell_width * this->size /
        this->fontdata->cell_height;
    s16 y2 = y1 - this->size;
    s16 s2 = s1 + this->fontdata->cell_width;
    s16 t2 = t1 + this->fontdata->cell_height;

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2s16(x1, y2); GX_Color1u32(c); GX_TexCoord2s16(s1, t1);
    GX_Position2s16(x2, y2); GX_Color1u32(c); GX_TexCoord2s16(s2, t1);
    GX_Position2s16(x2, y1); GX_Color1u32(c); GX_TexCoord2s16(s2, t2);
    GX_Position2s16(x1, y1); GX_Color1u32(c); GX_TexCoord2s16(s1, t2);
    GX_End();
}

int GX::Font::processString(const char *text,
bool shouldDraw, int &outY) {
    void *image;
    int32_t xpos, ypos, width, x, y;

    x = this->posX;
    y = this->posY;
    for (; *text != '\0'; text++) {
        char c = *text;
        switch(c) {
            case '\r':
                x = this->posX;
                break;

            case '\n':
                y += this->size;
                break;

            case ' ':
                //do not draw, because there's some crap on it
                SYS_GetFontTexture(c, &image, &xpos, &ypos, &width);
                x += width * this->size / this->fontdata->cell_height;
                break;

            default: {
                if (c < this->fontdata->first_char) continue;
                SYS_GetFontTexture(c, &image, &xpos, &ypos, &width);
                if(shouldDraw) {
                    this->drawCell(x, y, xpos, ypos);
                }
                x += width * this->size / this->fontdata->cell_height;
            }
        }
    }

    outY = y - this->posY;
    return x - this->posX;
}

int GX::Font::drawString(const char *text) {
    this->selectTexture();
    int dummyOutY;
    return this->processString(text, true, dummyOutY);
}

void GX::Font::measureString(const char *text, int &outW, int &outH) {
    outW = this->processString(text, false, outH);
}

}; //extern "C"
