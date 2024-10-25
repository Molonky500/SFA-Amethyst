#pragma once
#include <fstream>
#include <filesystem>

namespace GX {
    class Texture {
        public:
            typedef struct {
                //for simplicity's sake, we'll just use the same
                //file format as the game uses.
                void *next; //zero in file
                u32 flags; //zero in file
                s16 offset; //zero in file
                u16 width;
                u16 height;
                s16 usage; //reference count (1 in file)
                s16 frameVal10; //Low byte always 0 in file; overridden(?) on load; relates to number of frames
                s16 pad12;
                u16 framesPerTick; //how many frames to advance each tick
                u8 format; //GXTexFmt
                u8 wrapS;
                u8 wrapT;
                u8 minFilter;
                u8 magFilter; //Always 1, but doesn't necessarily have to be
                u8 pad1B;
                u8 minLod; //Minimum LOD value; bias is fixed at -2
                u8 maxLod; //Maximum LOD value; LOD is not used if maxLod <= minLod
                u8 unk1E; //Always 0 or 255, never accessed
                u8 pad1F;
                GXTexObj texObj; //GXTexObj; zero in file
                void *texRegion; //GXTexRegion*; zero in file
                s32 bufSize; //raw image data size
                u8 bNoTexRegionCallback; //use gxSetTexImage0 instead of gxCallTexRegionCallback
                u8 bDoNotFree; //maybe u8 memory region (N64 had multiple regions)
                s8 unk4A;
                s8 unk4B; //set to 10 when freeing, otherwise never accessed (memory region?)
                u32 bufSize2; //same as bufSize; seems to be "allocated size"; set but never read?
                u32 tevVal50; //0:use 1 TEV stage, not 2 (maybe bHasTwoTevStages?)
                u32 pad54[3];
            } FileHeader;

            Texture() {
                this->data = nullptr;
                this->width = 0;
                this->height = 0;
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
                this->data = new u8[header.bufSize];
                file.read(reinterpret_cast<char*>(this->data),
                    header.bufSize);
                file.close();
                DCStoreRange(this->data, header.bufSize);
                GX_InvalidateTexAll();

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

                GX_InitTexObj(&this->texObj, this->data,
                    this->width, this->height, this->format,
                    this->wrapS, this->wrapT, GX_FALSE);
                GX_InitTexObjLOD(&this->texObj, this->minFilter,
                    this->magFilter, this->minLod, this->maxLod, -2.0f,
                    GX_TRUE, GX_TRUE, GX_ANISO_1);
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
    };
};
