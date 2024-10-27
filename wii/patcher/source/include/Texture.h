#pragma once
#include <fstream>
#include <filesystem>
#include <new>

namespace GX {
    class Texture {
        public:
            static constexpr const u32 fmtBitsPerPixel(int fmt) {
                switch(fmt) {
                    case GX_TF_I4:     return  4;
                    case GX_TF_I8:     return  8;
                    case GX_TF_IA4:    return  8;
                    case GX_TF_IA8:    return 16;
                    case GX_TF_RGB565: return 16;
                    case GX_TF_RGB5A3: return 16;
                    case GX_TF_RGBA8:  return 32; //aka RGBA32, RGBA8888
                    case GX_TF_CI4:    return  4; //aka C4
                    case GX_TF_CI8:    return  8; //aka C8
                    case GX_TF_CI14:   return 16; //aka C14X2
                    case GX_TF_CMPR:   return  4; //aka BC1
                    default:           return  0;
                }
            }
            typedef struct {
                //for simplicity's sake, we'll just use the same
                //file format as the game uses.
                /* 0x00 */ void *next; //zero in file
                /* 0x04 */ u32 flags; //zero in file
                /* 0x08 */ s16 offset; //zero in file
                /* 0x0A */ u16 width;
                /* 0x0C */ u16 height;
                /* 0x0E */ s16 usage; //reference count (1 in file)
                /* 0x10 */ s16 frameVal10; //Low byte always 0 in file; overridden(?) on load; relates to number of frames
                /* 0x12 */ s16 pad12;
                /* 0x14 */ u16 framesPerTick; //how many frames to advance each tick
                /* 0x16 */ u8 format; //GXTexFmt
                /* 0x17 */ u8 wrapS;
                /* 0x18 */ u8 wrapT;
                /* 0x19 */ u8 minFilter;
                /* 0x1A */ u8 magFilter; //Always 1, but doesn't necessarily have to be
                /* 0x1B */ u8 pad1B;
                /* 0x1C */ u8 minLod; //Minimum LOD value; bias is fixed at -2
                /* 0x1D */ u8 maxLod; //Maximum LOD value; LOD is not used if maxLod <= minLod
                /* 0x1E */ u8 unk1E; //Always 0 or 255, never accessed
                /* 0x1F */ u8 pad1F;
                /* 0x20 */ GXTexObj texObj; //GXTexObj; zero in file
                /* 0x40 */ void *texRegion; //GXTexRegion*; zero in file
                /* 0x44 */ s32 bufSize; //raw image data size
                /* 0x48 */ u8 bNoTexRegionCallback; //use gxSetTexImage0 instead of gxCallTexRegionCallback
                /* 0x49 */ u8 bDoNotFree; //maybe u8 memory region (N64 had multiple regions)
                /* 0x4A */ s8 unk4A;
                /* 0x4B */ s8 unk4B; //set to 10 when freeing, otherwise never accessed (memory region?)
                /* 0x4C */ u32 bufSize2; //same as bufSize; seems to be "allocated size"; set but never read?
                /* 0x50 */ u32 tevVal50; //0:use 1 TEV stage, not 2 (maybe bHasTwoTevStages?)
                /* 0x54 */ u32 pad54[3];
                /* 0x60 end */
            } FileHeader;
            static_assert(sizeof(FileHeader) == 0x60);

            Texture() {
                this->data = nullptr;
                this->width = 0;
                this->height = 0;
            }

            Texture(u16 width, u16 height, u8 fmt, void *data) {
                this->data   = (u8*)data;
                this->width  = width;
                this->height = height;
                this->format = fmt;
                this->wrapS  = GX_REPEAT;
                this->wrapT  = GX_REPEAT;
                this->minFilter = 1;
                this->magFilter = 1;
                this->minLod = GX_NEAR;
                this->maxLod = GX_NEAR;
                this->dataLen = width * height * fmtBitsPerPixel(fmt);
                this->createTexObj();
            }

            Texture(std::filesystem::path path) {
                FileHeader header;
                std::ifstream file(path, std::ios::binary);
                auto length = std::filesystem::file_size(path);
                if(length < sizeof(header)) {
                    file.close();
                    throw std::runtime_error("Not valid texture file");
                }
                file.read(reinterpret_cast<char*>(&header),
                    sizeof(header));
                this->data = new (std::align_val_t(32)) u8[header.bufSize];
                memset(this->data, 0, header.bufSize);
                file.read(reinterpret_cast<char*>(this->data),
                    header.bufSize);
                file.close();
                DCStoreRange(this->data, header.bufSize);
                GX_InvalidateTexAll();

                printf("Texture dims=%dx%d fmt=0x%X wrap=%d,%d filt=%d,%d "
                    "lod=%d,%d size=%d\r\n",
                    header.width, header.height, header.format,
                    header.wrapS, header.wrapT,
                    header.minFilter, header.magFilter,
                    header.minLod, header.maxLod, header.bufSize);

                this->width     = header.width;
                this->height    = header.height;
                this->format    = header.format;
                this->wrapS     = header.wrapS;
                this->wrapT     = header.wrapT;
                this->minFilter = header.minFilter;
                this->magFilter = header.magFilter;
                this->minLod    = header.minLod;
                this->maxLod    = header.maxLod;
                this->dataLen   = header.bufSize;
                this->createTexObj();
            }

            ~Texture() {
                if(this->data) delete[] this->data;
            }

            void select(u8 map=GX_TEXMAP0) {
                GX_LoadTexObj(&this->texObj, map);
            }

            u32 getSize(u16 *outW, u16 *outH) {
                if(outW) *outW = this->width;
                if(outH) *outH = this->height;
                return this->dataLen;
            }

        protected:
            u8 *data;
            u32 dataLen;
            u16 width, height;
            u8 format;
            u8 wrapS, wrapT;
            u8 minFilter, magFilter;
            float minLod, maxLod;
            GXTexObj texObj;

            void createTexObj() {
                GX_InitTexObj(&this->texObj, this->data,
                    this->width, this->height, this->format,
                    this->wrapS, this->wrapT, GX_FALSE);
                GX_InitTexObjLOD(&this->texObj, this->minFilter,
                    this->magFilter, this->minLod, this->maxLod, -2.0f,
                    GX_TRUE, GX_TRUE, GX_ANISO_1);
            }
    };

    extern Texture gBlankTexture;
};
