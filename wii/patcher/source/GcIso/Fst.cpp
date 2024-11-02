#include "main.h"
#include "GcIso/Iso.h"
#include "GcIso/Fst.h"

GcIso::Fst::Fst(GcIso::Iso *iso) {
    this->root = nullptr;
    fseek(iso->file, iso->header.bootBin.fstOffs, SEEK_SET);

    auto nEntries = this->_readRoot(iso->file);
    off_t strTabOffs = (nEntries * sizeof(EntryStruct)) +
        iso->header.bootBin.fstOffs;

    //read entries
    fseek(iso->file, iso->header.bootBin.fstOffs +
        sizeof(EntryStruct), SEEK_SET);
    EntryStruct entries[nEntries-1];
    fread(entries, sizeof(EntryStruct), nEntries-1, iso->file);

    //read string table and construct entries
    for(unsigned int i=1; i<nEntries; i++) {
        Entry *ent;
        if(entries[i].nameOffs & 0xFF000000) ent = new DirEntry();
        else ent = new FileEntry();
        ent->name = this->_readString(iso->file,
            strTabOffs + (entries[i].nameOffs & 0xFFFFFF));
        ent->idx = i;
        this->entriesByIndex.push_back(ent);
    }
    this->_assignChildren();
}

GcIso::Fst::~Fst() {
    if(this->root) delete this->root;
}

u32 GcIso::Fst::_readRoot(FILE *file) {
    //read root entry, which must be a directory
    EntryStruct esRoot;
    fread(&esRoot, sizeof(EntryStruct), 1, file);
    int nEntries = esRoot.dir.nextIdx; //includes this entry

    this->root = new DirEntry();
    this->root->name = "";
    this->root->idx = 0;
    this->root->parentIdx = -1;
    this->root->nextIdx = -1;
    this->entriesByIndex.push_back(this->root);

    return nEntries;
}

std::string GcIso::Fst::_readString(FILE *file, off_t offset) {
    fseek(file, offset, SEEK_SET);
    std::string str = "";
    while(1) {
        char c;
        int r = fread(&c, 1, 1, file);
        if(!c || !r) break;
        str += c;
    }
    return str;
}

void GcIso::Fst::_assignChildren() {
    auto nEntries = this->entriesByIndex.size();
    for(unsigned int i=1; i<nEntries; i++) {
        auto ent = this->entriesByIndex[i];
        ((DirEntry*)this->entriesByIndex[i])->children[ent->name] = ent;
    }
}

GcIso::Fst::Entry* GcIso::Fst::find(std::filesystem::path path) {
    GcIso::Fst::Entry *entry = this->root;
    for(auto it = path.begin(); it != path.end(); ++it) {
        std::string name(*it);
        //technically should only accept "dvd:" at the beginning
        //and blank at the end...
        if(name == "" || name == "dvd:") continue;
        entry = ((DirEntry*)entry)->children[name];
        if(!(entry && entry->isDir)) break;
        //XXX this means eg "/foo/bar/baz/butt" is a valid
        //path if "/foo/bar" is a file
    }
    return entry;
}
