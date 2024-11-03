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

namespace GcIso {
    /**
     * @brief FILE object for ISO.
     * @details This is instantiated by open("dvd:/...").
     *  (assuming an ISO is mounted as "dvd".)
     *  You should not instantiate/use it directly.
     */
    class File {
        public:
            static constexpr const int MAX_PATH = 256;
            File(GcIso::Iso *iso, const char *path, int flags, int mode);
            ~File() { }

            ssize_t read(struct _reent *r, char *ptr, size_t len);
            off_t seek(struct _reent *r, off_t pos, int dir);
            int fstat(struct _reent *r, struct stat *st);
            int stat(struct _reent *r, struct stat *st);

        protected:
            char path[MAX_PATH];
            GcIso::Iso *iso;
            int flags; //from open()
            int mode; //from open()
            bool isDir;
            off_t offset; //current read/write position
            off_t size;
            u32 sector; //start sector

            static void _initIoWrapper();
            int _getMode();
    };
};
