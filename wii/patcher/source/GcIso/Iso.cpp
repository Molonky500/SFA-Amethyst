#include "main.h"

GcIso::Iso::Iso(std::filesystem::path path, std::string mode) {
    //path can be just "dvd:/" to open the actual disc in the drive.
    //it could also be a file on the SD card, or even a file on
    //the inserted disc.
    this->file = fopen(path.c_str(), mode.c_str());
    if(!this->file) {
        int err = errno;
        throw new std::filesystem::filesystem_error(
            strerror(err), path, err);
    }

    if(mode.find("r")) this->_readHeader();
}

GcIso::Iso::~Iso() {
    if(this->file) fclose(this->file);
}

void GcIso::Iso::_readHeader() {
    auto r = fread(&this->header,
        sizeof(GcIso::Iso::Header), 1, this->file);
    if(r < sizeof(GcIso::Iso::Header)
    || this->header->bootBin.magic != GcIso::BootBin::MAGIC) {
        fprintf(stderr, "ISO header: read %d, magic 0x%08X\r\n",
            r, this->header->bootBin.magic);
        throw new std::runtime_error("Malformed ISO file");
    }

    this->fst = new GcIso::Fst(this);
}
