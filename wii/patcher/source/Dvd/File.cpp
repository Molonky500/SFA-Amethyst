#include "main.h"
#include "Dvd/File.h"

//this interface is specifically for raw DVD access.
//for accessing the filesystems, use GcIso("dvdraw")::mount("someName").

Sys::Dvd::File::File(int flags, int mode) {
    this->flags  = flags;
    this->mode   = mode;
    this->offset = 0;
    this->size   = 1'459'978'240; //XXX don't hardcode
}

ssize_t Sys::Dvd::File::read(struct _reent *r,  char *ptr, size_t len) {
    off_t offEnd = this->offset + len;
    if(offEnd >= this->size) {
        len -= (offEnd - this->size)+1;
        printf("length truncated to %zd\r\n", len);
    }
    //printf("DVD::File::read(%p, %p, %zd)\r\n", r, ptr, len);
    ssize_t res = gApp->dvd->read(ptr, len, this->offset);
    if(res >= 0) {
        this->offset += res;
        return res;
    }
    else {
        r->_errno = res;
        return -1;
    }
}

off_t Sys::Dvd::File::seek(struct _reent *r, off_t pos, int dir) {
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
    //printf("DVD::File::seek to 0x%X\r\n", (u32)this->offset);
    return 0;
}

int Sys::Dvd::File::fstat(struct _reent *r, struct stat *st) {
    st->st_size = this->size;
	st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
	return 0;
}

int Sys::Dvd::File::stat(struct _reent *r, struct stat *st) {
    printf("Dvd::File::stat(%p)\r\n", st);
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

static int __dvd_open(struct _reent *r, void *fileStruct,
const char *path, int flags, int mode) {
    printf("__dvd_open(%s)\r\n", path);
    if((flags & 3) != O_RDONLY) { //DVD drive is read-only
        r->_errno = EROFS;
		return -1;
    }
    //path is ignored, could be used for something
    new(fileStruct) Sys::Dvd::File(flags, mode);
    return 0;
}

static int __dvd_close(struct _reent *r, void *fd) {
    auto file = (Sys::Dvd::File*)fd;
    //with placement new, must manually call destructor
    file->~File();
    return 0;
}

static ssize_t __dvd_read(struct _reent *r, void *fd,
char *ptr, size_t len) {
    //printf("__dvd_read(%p, %d)\r\n", ptr, len);
    return ((Sys::Dvd::File*)fd)->read(r, ptr, len);
}

static off_t __dvd_seek(struct _reent *r, void *fd, off_t pos, int dir) {
    return ((Sys::Dvd::File*)fd)->seek(r, pos, dir);
}

static int __dvd_fstat(struct _reent *r, void *fd, struct stat *st) {
    if(!fd) {
		r->_errno = EBADF;
		return -1;
	}
    return ((Sys::Dvd::File*)fd)->fstat(r, st);
}

static devoptab_t devDvd = {
    "dvdraw",    //name
	sizeof(Sys::Dvd::File), //structSize
	__dvd_open,  //open_r
	__dvd_close, //close_r
	NULL,        //write_r
	__dvd_read,  //read_r
	__dvd_seek,  //seek_r
	__dvd_fstat, //fstat_r
	NULL,        //stat_r
	NULL,        //link_r
	NULL,        //unlink_r
	NULL,        //chdir_r
	NULL,        //rename_r
	NULL,        //mkdir_r
	0,           //dirStateSize
	NULL,        //diropen_r
	NULL,        //dirreset_r
	NULL,        //dirnext_r
	NULL,        //dirclose_r
	NULL,        //statvfs_r
	NULL,        //ftruncate_r
	NULL,        //fsync_r
	NULL,        //deviceData
	NULL,        //chmod_r
	NULL,        //fchmod_r
};
void Sys::Dvd::File::_initIoWrapper() {
	int r = AddDevice(&devDvd);
    printf("AddDevice => %d, %d\r\n", r, errno);
}
