#include "main.h"
#include "GcIso/Iso.h"
#include "GcIso/File.h"

const char* removeDeviceFromPath(const char *path) {
    for(int i=0; i<MAX_DEV_NAME_LEN; i++) {
        if(path[i] == ':') {
            return &path[i+1];
            break;
        }
        else if(path[i] == '\0') break;
    }
    return path;
}

static std::map<std::string, GcIso::Iso*> mountedIsos;

static GcIso::Iso* getMountedIso(const char *path) {
    //extract the device
    char devName[MAX_DEV_NAME_LEN+1];
    for(int i=0; i<MAX_DEV_NAME_LEN && path[i]; i++) {
        if(path[i] == ':') break;
        devName[i] = path[i];
        devName[i+1] = '\0';
    }

    try {
        return mountedIsos.at(std::string(devName));
    }
    catch(std::out_of_range &ex) {
        throw new std::runtime_error(
            std::string("No ISO mounted at: ") + devName);
    }
}

GcIso::Iso::Iso(std::filesystem::path path, std::string mode) {
    this->fst = nullptr;
    this->devName[0] = '\0';
    this->file = fopen(path.c_str(), mode.c_str());
    if(!this->file) {
        int err = errno;
        throw new std::filesystem::filesystem_error(
            std::string(strerror(err)), path,
            std::error_code(err, std::system_category()));
    }

    if(mode.find("r") != std::string::npos) this->_readHeader();
    else printf("Mode is \"%s\", not reading header\r\n", mode.c_str());
}

GcIso::Iso::~Iso() {
    if(this->file) fclose(this->file);
}

void GcIso::Iso::_readHeader() {
    fseek(this->file, 0, SEEK_SET);
    auto r = fread(&this->header,
        1, sizeof(GcIso::Iso::Header), this->file);
    if(r < sizeof(GcIso::Iso::Header)
    || this->header.bootBin.magic != GcIso::BootBin::MAGIC) {
        fprintf(stderr, "ISO header: read %d/%d, magic 0x%08X/0x%08X\r\n",
            r, sizeof(GcIso::Iso::Header),
            this->header.bootBin.magic, GcIso::BootBin::MAGIC);
        throw new std::runtime_error("Malformed ISO file");
    }

    this->fst = new GcIso::Fst(this);
}

static int __iso_open(struct _reent *r, void *fileStruct,
const char *path, int flags, int mode) {
    printf("__iso_open(%s)\r\n", path);
    const char *filePath = removeDeviceFromPath(path);

    try {
        auto iso = getMountedIso(path);
        new(fileStruct) GcIso::File(iso, filePath, flags, mode);
        return 0;
    }
    catch(std::filesystem::filesystem_error &err) {
        fprintf(stderr, "__iso_open(%s): %d %s\r\n", path,
            err.code().value(), err.what());
        r->_errno = err.code().value();
        return -1;
    }
}

static int __iso_close(struct _reent *r, void *fd) {
    auto file = (GcIso::File*)fd;
    //with placement new, must manually call destructor
    file->~File();
    return 0;
}

static ssize_t __iso_read(struct _reent *r, void *fd,
char *ptr, size_t len) {
    return ((GcIso::File*)fd)->read(r, ptr, len);
}

static off_t __iso_seek(struct _reent *r, void *fd, off_t pos, int dir) {
    return ((GcIso::File*)fd)->seek(r, pos, dir);
}

static int __iso_fstat(struct _reent *r, void *fd, struct stat *st) {
    if(!fd) {
		r->_errno = EBADF;
		return -1;
	}
    return ((GcIso::File*)fd)->fstat(r, st);
}

static int __iso_stat(struct _reent *r, const char *path, struct stat *st) {
    if(path == NULL) {
		r->_errno = ENODEV;
		return -1;
	}
    if(strcmp(path,".")==0 || strcmp(path,"..")==0) {
		memset(st, 0, sizeof(struct stat));
		st->st_mode = S_IFDIR;
		return 0;
	}

    auto iso = getMountedIso(path);
    GcIso::File file(iso, path, O_RDONLY, 0);
    return file.stat(r, st);
}

static int __iso_chdir(struct _reent *r, const char *path) {
	path = removeDeviceFromPath(path);
    //TODO more...
    r->_errno = ENOSYS;
    return -1;
}

bool GcIso::Iso::mount(std::string name) {
    if(this->devName[0] != '\0') {
        if(!strncmp(this->devName, name.c_str(), MAX_DEV_NAME_LEN)) {
            return true;
        }
        throw new std::runtime_error("Already mounted");
    }
    strncpy(this->devName, name.c_str(), MAX_DEV_NAME_LEN);
    this->devoptab.name         = this->devName;
    printf("Mount ISO at \"%s\"\r\n", this->devName);
	this->devoptab.structSize   = sizeof(GcIso::File);
	this->devoptab.open_r       = __iso_open;
	this->devoptab.close_r      = __iso_close;
	this->devoptab.write_r      = NULL;
	this->devoptab.read_r       = __iso_read;
	this->devoptab.seek_r       = __iso_seek;
	this->devoptab.fstat_r      = __iso_fstat;
	this->devoptab.stat_r       = __iso_stat;
	this->devoptab.link_r       = NULL;
	this->devoptab.unlink_r     = NULL;
	this->devoptab.chdir_r      = __iso_chdir;
	this->devoptab.rename_r     = NULL;
	this->devoptab.mkdir_r      = NULL;
	this->devoptab.dirStateSize = 0;
	this->devoptab.diropen_r    = NULL;
	this->devoptab.dirreset_r   = NULL;
	this->devoptab.dirnext_r    = NULL;
	this->devoptab.dirclose_r   = NULL;
	this->devoptab.statvfs_r    = NULL;
	this->devoptab.ftruncate_r  = NULL;
	this->devoptab.fsync_r      = NULL;
	this->devoptab.deviceData   = NULL;
	this->devoptab.chmod_r      = NULL;
	this->devoptab.fchmod_r     = NULL;
    int r = AddDevice(&this->devoptab);
    printf("AddDevice => %d, %d\r\n", r, errno);
    if(r < 0) {
        fprintf(stderr, "AddDevice error %d\r\n", errno);
    }
    mountedIsos[name] = this;
    return true;
}

void GcIso::Iso::unmount() {
    printf("Unmount ISO from \"%s\"\r\n", this->devName);
    RemoveDevice(this->devName);
    mountedIsos[std::string(this->devName)] = nullptr;
    this->devName[0] = '\0';
}
