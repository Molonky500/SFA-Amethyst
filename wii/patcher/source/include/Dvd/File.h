#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

namespace Sys { namespace Dvd {
    /**
     * @brief FILE object for DVD.
     * @details This is instantiated by open("dvd:/...").
     *  You should not instantiate/use it directly.
     */
    class File {
        public:
            constexpr const int PATH_MAX = 256;
            File(const char *path, int flags, int mode);
            ~File() { }

            ssize_t read(struct _reent *r, char *ptr, size_t len);
            off_t seek(struct _reent *r, off_t pos, int dir);
            int fstat(struct _reent *r, struct stat *st);
            static int Sys::Dvd::File::stat(struct _reent *r, struct stat *st);

        protected:
            friend class Device;
            char path[GCDVD_PATH_MAX];
            int flags; //from open()
            int mode; //from open()
            bool isDir;
            off_t offset; //current read/write position
            off_t size;
            u32 sector; //start sector

            static void _initIoWrapper();
            int _getMode();
    };
}};
