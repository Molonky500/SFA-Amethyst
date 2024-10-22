#include "main.h"

sys_fontheader *fontdata;
GXTexObj fonttex;
int text_x;
int text_y;
int text_size;
u32 text_color = 0xffffffff;

inline void fontSetPos(int x, int y) {
    text_x = x;
    text_y = y;
}

inline void fontSetSize(int size) {
    text_size = size;
}

inline void fontSetColor(Color4b color) {
    text_color = color.value;
}

static void activate_font_texture() {
    u32 texture_size;
    void *texels;

    texels = (void *)fontdata + fontdata->sheet_image;
    GX_InitTexObj(&fonttex, texels,
                  fontdata->sheet_width, fontdata->sheet_height,
                  fontdata->sheet_format, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&fonttex, GX_LINEAR, GX_LINEAR, 0., 0., 0.,
                     GX_TRUE, GX_TRUE, GX_ANISO_1);
    GX_LoadTexObj(&fonttex, GX_TEXMAP0);

    texture_size = GX_GetTexBufferSize(fontdata->sheet_width,
                                       fontdata->sheet_height,
                                       fontdata->sheet_format,
                                       GX_FALSE, 0);
    DCStoreRange(texels, texture_size);
    GX_InvalidateTexAll();
}

void appFontInit() {
    if (SYS_GetFontEncoding() == 0) {
        fontdata = memalign(32, SYS_FONTSIZE_ANSI);
    } else {
        fontdata = memalign(32, SYS_FONTSIZE_SJIS);
    }

    SYS_InitFont(fontdata);
    activate_font_texture();
    text_size = fontdata->cell_height;
}

static void draw_font_cell(int16_t x1, int16_t y1,
uint32_t c, int16_t s1, int16_t t1) {
    int16_t x2 = x1 + fontdata->cell_width * text_size /
        fontdata->cell_height;
    int16_t y2 = y1 - text_size;
    int16_t s2 = s1 + fontdata->cell_width;
    int16_t t2 = t1 + fontdata->cell_height;

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2s16(x1, y2); GX_Color1u32(c); GX_TexCoord2s16(s1, t1);
    GX_Position2s16(x2, y2); GX_Color1u32(c); GX_TexCoord2s16(s2, t1);
    GX_Position2s16(x2, y1); GX_Color1u32(c); GX_TexCoord2s16(s2, t2);
    GX_Position2s16(x1, y1); GX_Color1u32(c); GX_TexCoord2s16(s1, t2);
    GX_End();
}

static int process_string(const char *text, bool should_draw, int *outY) {
    void *image;
    int32_t xpos, ypos, width, x, y;

    x = text_x;
    y = text_y;
    for (; *text != '\0'; text++) {
        char c = *text;
        switch(c) {
            case '\r':
                x = text_x;
                break;

            case '\n':
                y += text_size;
                break;

            default: {
                if (c < fontdata->first_char) {
                    continue;
                }
                SYS_GetFontTexture(c, &image, &xpos, &ypos, &width);
                if (should_draw) {
                    draw_font_cell(x, y, text_color, xpos, ypos);
                }
                x += width * text_size / fontdata->cell_height;
            }
        }
    }

    if(outY) *outY = y - text_y;
    return x - text_x;
}

int fontDrawString(const char *text) {
    return process_string(text, true, NULL);
}

void fontMeasureString(const char *text, int *outX, int *outY) {
    int r = process_string(text, false, outY);
    if(outX) *outX = r;
}
