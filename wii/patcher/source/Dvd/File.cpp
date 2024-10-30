#include "main.h"
#include "dvd/DvdFile.h"

Sys::Dvd::Device *gDvd = nullptr;

Sys::Dvd::File::File(const char *path, int flags, int mode) {
    if(!gDvd) gDvd = new Sys::Dvd::Device();
    if(strlen(path) >= PATH_MAX) {
        throw new std::runtime_error("Path too long");
    }
    strncpy(this->path, path, PATH_MAX);
    this->flags = flags;
    this->mode  = mode;
    this->offset = 0;
    this->len = 0;
    //TODO: find this file on the disc
    this->sector = 0;
    this->isDir = false;
}

static int __dvd_open(struct _reent *r, void *fileStruct,
const char *path, int flags, int mode) {
    if(flags & 3 != O_RDONLY) { //DVD drive is read-only
        r->_errno = EROFS;
		return -1;
    }

    char fixedpath[Sys::Dvd::File::PATH_MAX];
	ExtractDevice(path, fixedpath);
	if(fixedpath[0]=='\0') {
		getcwd(fixedpath, Sys::Dvd::File::PATH_MAX);
		ExtractDevice(fixedpath, fixedpath);
	}
    printf("__dvd_open fixedpath=\"%s\"\r\n", fixedpath);
    auto file = new(fileStruct) Sys::Dvd::File(fixedpath, flags, mode);
    return 0;
}

static int __dvd_close(struct _reent *r, void *fd) {
    auto file = (Sys::Dvd::File*)fd;
    //with placement new, must manually call destructor
    file->~Sys::Dvd::File();
}

ssize_t Sys::Dvd::File::read(struct _reent *r,  char *ptr, size_t len) {
    //XXX don't read past end of file
    return gDvd->read(ptr, len, this->offset);
}
static ssize_t __dvd_read(struct _reent *r, void *fd,
char *ptr, size_t len) {
    return ((Sys::Dvd::File*)fd)->read(r, ptr, len);
}

off_t Sys::Dvd::File::seek(struct _reent *r, off_t pos, int dir) {
    off_t position;
    switch (dir) {
        case SEEK_SET: position = pos; break;
        case SEEK_CUR:
            position = file->offset + pos;
            break;
        case SEEK_END:
            position = file->len + pos;
            break;
        default:
            r->_errno = EINVAL;
            _SMB_unlock(file->env);
            return -1;
	}
}
static off_t __dvd_seek(struct _reent *r, void *fd, off_t pos, int dir) {
    return ((Sys::Dvd::File*)fd)->seek(r, pos, dir);
}

int Sys::Dvd::File::_getMode() {
    return
        (this->isDir ? (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH) : S_IFREG)
        | S_IRUSR | S_IRGRP | S_IROTH;
}
int Sys::Dvd::File::fstat(struct _reent *r, struct stat *st) {
    st->st_size = this->len;
	st->st_mode =

	return 0;
}
static int __dvd_fstat(struct _reent *r, void *fd, struct stat *st) {
    if(!fd) {
		r->_errno = EBADF;
		return -1;
	}
    return ((Sys::Dvd::File*)fd)->fstat(r, st);
}

int Sys::Dvd::File::stat(struct _reent *r, struct stat *st) {
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
static int __dvd_stat(struct _reent *r, const char *path, struct stat *st) {
    if(!path == NULL) {
		r->_errno = ENODEV;
		return -1;
	}
    if(strcmp(path,".")==0 || strcmp(path,"..")==0) {
		memset(st, 0, sizeof(struct stat));
		st->st_mode = S_IFDIR;
		return 0;
	}

    Sys::Dvd::File file(path, O_RDONLY, 0);
    return file->stat(r, st);
}

static int __dvd_chdir(struct _reent *r, const char *path) {
	char path_absolute[Sys::Dvd::File::PATH_MAX];
	ExtractDevice(path,path_absolute);
	if(path_absolute[0]=='\0') {
		getcwd(path_absolute,SMB_MAXPATH);
		ExtractDevice(path_absolute,path_absolute);
	}
    //TODO more...
    r->_errno = ENOSYS;
    return -1;
}

static const devoptab_t devDvd = {
	"dvd",	     // device name
	sizeof(DvdFile), // size of file structure
	__dvd_open,  // device open
	__dvd_close, // device close
	NULL,        // device write
	__dvd_read,  // device read
	__dvd_seek,  // device seek
	__dvd_fstat, // device fstat
	__dvd_stat,  // device stat
	NULL,        // device link
	NULL,        // device unlink
	__dvd_chdir, // device chdir
	NULL,        // device rename
	NULL,        // device mkdir
	0,           // dirStateSize
	NULL,        // device diropen_r
	NULL,        // device dirreset_r
	NULL,        // device dirnext_r
	NULL,        // device dirclose_r
	NULL         // device statvfs_r
};

void Sys::Dvd::File::_initIoWrapper() {
	AddDevice(devDvd);
}
