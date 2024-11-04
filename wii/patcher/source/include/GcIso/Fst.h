#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
};

#include <map>
#include <string>
#include <vector>

namespace GcIso {
    class Iso;
    class Fst {
        public:
            typedef struct {
                u32 nameOffs; //u8 type (0:file 1:dir), u24 nameOffs
                union {
                    struct {
                        u32 fileOffs;
                        u32 size;
                    } file;
                    struct {
                        u32 parentIdx;
                        u32 nextIdx; //next idx which is NOT in this dir
                    } dir;
                };
            } EntryStruct;

            class Entry {
                public:
                    std::string name;
                    int idx;
                    bool isDir;
            };

            class FileEntry: public Entry {
                public:
                    off_t offset, size;
                    FileEntry(): Entry() { this->isDir = false; }
            };
            class DirEntry: public Entry {
                public:
                    int parentIdx, nextIdx;
                    std::map<std::string, Entry*> children;
                    DirEntry(): Entry() { this->isDir = true; }
            };

            DirEntry *root;

            Fst(Iso *iso);
            ~Fst();
            Entry* find(std::filesystem::path path);

        protected:
            std::vector<Entry*> entriesByIndex;

            u32 _readRoot(FILE *file);
            std::string _readString(FILE *file, off_t offset);
            int _assignChildren(int idx, Entry *ent, int depth);
    };
};
