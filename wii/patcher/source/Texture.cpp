#include "main.h"
const u16 _blankTextureData = 0;
GX::Texture GX::gBlankTexture(1, 1, GX_TF_RGB565, (void*)&_blankTextureData);
