#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/iosupport.h>
};

#include <filesystem>
#include <string>
#include "GcIso/Appldr.h"
#include "GcIso/Bi2Bin.h"
#include "GcIso/BootBin.h"
#include "GcIso/Fst.h"

const char* removeDeviceFromPath(const char *path);

namespace GcIso {
    class Iso {
        public:
            typedef struct {
                BootBin::Struct bootBin;
                Bi2Bin::Struct bi2bin;
                Appldr::Header appldrHeader;
                //appldr
                //fst
                //main.dol
                //files
            } Header;

            Iso(std::filesystem::path path, std::string mode="rb");
            ~Iso();
            bool mount(std::string name);
            void unmount();

        protected:
            friend class Fst;
            friend class File;
            Fst *fst;
            FILE *file;
            Header header;
            devoptab_t devoptab;
            //std::string c_str() does not return a string that
            //remains valid later, so copy it here
            char devName[MAX_DEV_NAME_LEN];

            void _readHeader();
    };
};
