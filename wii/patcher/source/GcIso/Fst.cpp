#include "main.h"
#include "GcIso/Iso.h"
#include "GcIso/Fst.h"

GcIso::Fst::Fst(GcIso::Iso *iso) {
    this->root = nullptr;
    fseek(iso->file, iso->header.bootBin.fstOffs, SEEK_SET);

    auto nEntries = this->_readRoot(iso->file);
    off_t strTabOffs = ((nEntries+1) * sizeof(EntryStruct)) +
        iso->header.bootBin.fstOffs;

    //read entries
    fseek(iso->file, iso->header.bootBin.fstOffs +
        sizeof(EntryStruct), SEEK_SET);
    EntryStruct entries[nEntries-1];
    memset(entries, 0, sizeof(EntryStruct) * (nEntries-1));
    fread(entries, sizeof(EntryStruct), nEntries-1, iso->file);

    //determine maximum string table offset
    u32 maxStrOffs = 0;
    for(unsigned int i=0; i<nEntries-1; i++) {
        maxStrOffs = MAX(maxStrOffs, entries[i].nameOffs & 0xFFFFFF);
    }
    //assume the last name isn't crazy long
    maxStrOffs += 1024;

    //read string table
    char *strTab = new char[maxStrOffs];
    fseek(iso->file, strTabOffs, SEEK_SET);
    printf("Read strtab (0x%X)...\r\n", maxStrOffs);
    fread(strTab, 1, maxStrOffs, iso->file);
    printf("Read OK\r\n");

    //read string table and construct entries
    for(unsigned int i=0; i<nEntries-1; i++) {
        Entry *ent;
        if(entries[i].nameOffs & 0xFF000000) ent = new DirEntry(entries[i]);
        else ent = new FileEntry(entries[i]);
        ent->name = std::string(&strTab[entries[i].nameOffs & 0xFFFFFF]);
        ent->idx = i;
        //printf("name %d is \"%s\"\r\n", i, ent->name.c_str());
        this->entriesByIndex.push_back(ent);
    }
    delete[] strTab;
    printf("Assign children...\r\n");
    this->_assignChildren(0, this->root, 0);
    printf("FST read OK\r\n");

    /*printf("Files in root:\r\n");
    for(const auto& [key, value] : this->root->children) {
        printf("%s %p\r\n", key.c_str(), value);
    }
    printf("end\r\n");*/
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
    this->root->nextIdx = nEntries;
    this->entriesByIndex.push_back(this->root);

    return nEntries - 1;
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

int GcIso::Fst::_assignChildren(int idx, GcIso::Fst::DirEntry *ent, int depth) {
    if(depth >= 10) {
        throw new std::runtime_error("Directory nesting too deep");
    }
    if(!ent->isDir) {
        fprintf(stderr, "Entry %d (%s) is not a directory\r\n", idx,
            ent->name.c_str());
        return 0x7FFFFFFF;
    }
    idx++;
    int nEntries = this->entriesByIndex.size();
    //printf("%*schild[%d] (%s) next %d parent %d\r\n", depth+1, "-",
    //    idx, ent->name.c_str(), ent->nextIdx, ent->parentIdx);
    while(idx < ent->nextIdx && idx < nEntries) {
        auto f = this->entriesByIndex[idx];
        //printf("%*s[%c %s]\r\n", depth+2, "-",
        //    f->isDir ? 'D' : 'F', f->name.c_str());
        ent->children[f->name] = f;
        if(f->isDir) idx = this->_assignChildren(idx, (DirEntry*)f, depth+1);
        else idx++;
    }
    return idx;
}

GcIso::Fst::Entry* GcIso::Fst::find(std::filesystem::path path) {
    printf("FST find %s, this=%p\r\n", path.c_str(), this);
    GcIso::Fst::Entry *entry = this->root;
    printf("entry start %p\r\n", entry);
    for(auto it = path.begin(); it != path.end(); ++it) {
        std::string name = (*it).generic_string();
        printf("Check \"%s\"\r\n", name.c_str());
        //technically should only accept "dvd:" or "/" at the beginning
        //and blank at the end...
        if(name == "" || name == "/"
        || name.find(':') != std::string::npos) continue;
        entry = ((DirEntry*)entry)->children[name];
        if(!(entry && entry->isDir)) break;
        //XXX this means eg "/foo/bar/baz/butt" is a valid
        //path if "/foo/bar" is a file (what IS the correct
        //behaviour in this case?)
    }
    printf("Result %p\r\n", entry);
    return entry;
}
