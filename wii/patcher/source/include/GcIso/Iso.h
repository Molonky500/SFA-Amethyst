#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

#include <filesystem>
#include <string>
#include "Gciso/Appldr.h"
#include "Gciso/Bi2Bin.h"
#include "Gciso/BootBin.h"

namespace GcIso {
    class Iso {
        public:
            typedef struct {
                BootBin::Struct bootBin;
                Bi2Bin::Struct bi2bin;
                AppldrHeader::Header appldrHeader;
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
            FILE *file;
            Header header;
    }
};
