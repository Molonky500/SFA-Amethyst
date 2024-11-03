#include "main.h"
#include "GcIso/File.h"
#include "GcIso/Fst.h"

GcIso::File::File(GcIso::Iso *iso, const char *path, int flags, int mode) {
    if(strlen(path) >= GcIso::File::MAX_PATH) {
        throw new std::runtime_error("Path too long");
    }
    strncpy(this->path, path, GcIso::File::MAX_PATH);
    this->iso    = iso;
    this->flags  = flags;
    this->mode   = mode;
    this->offset = 0;
    this->size   = 65536;
    this->sector = 0;
    this->isDir  = false;
}

ssize_t GcIso::File::read(struct _reent *r,  char *ptr, size_t len) {
    off_t offEnd = this->offset + len;
    if(offEnd >= this->size) {
        len -= (offEnd - this->size)+1;
    }
    fseek(this->iso->file, this->offset +
        (this->sector * Sys::Dvd::SECTOR_SIZE), SEEK_SET);
    ssize_t res = fread(ptr, 1, len, this->iso->file);
    if(res >= 0) this->offset += res;
    return res;
}

off_t GcIso::File::seek(struct _reent *r, off_t pos, int dir) {
    switch (dir) {
        case SEEK_SET: this->offset = pos; break;
        case SEEK_CUR:
            this->offset = this->offset + pos;
            break;
        case SEEK_END:
            this->offset = this->size + pos;
            break;
        default:
            r->_errno = EINVAL;
            return -1;
	}
    if(this->offset >= this->size) this->offset = this->size - 1;
    return 0;
}

int GcIso::File::_getMode() {
    return
        (this->isDir ? (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH) : S_IFREG)
        | S_IRUSR | S_IRGRP | S_IROTH;
}
int GcIso::File::fstat(struct _reent *r, struct stat *st) {
    st->st_size = this->size;
	st->st_mode = this->_getMode();
	return 0;
}

int GcIso::File::stat(struct _reent *r, struct stat *st) {
    st->st_dev = 0; //TODO what goes here?
    st->st_ino = 0; //TODO: inode number
    st->st_mode = this->_getMode();
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = this->size;
    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0;
    st->st_blksize = 2048; //DVD sector size
    st->st_blocks = (this->size+2047) / 2048;
    return 0;
}
