#include "main.h"
#include "revolution/os.h"

void initEndHook(const char *msg) {
    //msg = "finished init"
    u32 totalBlocks, totalBytes, usedBlocks, usedBytes;
    int blocksPct, bytesPct;
    getFreeMemory(&totalBlocks, &totalBytes, &usedBlocks, &usedBytes,
        &blocksPct, &bytesPct);
    OSReport("Init done; bytes used %d/%d, blocks %d/%d, took %f sec\n",
        usedBytes, totalBytes, usedBlocks, totalBlocks,
        ticksToSecs(__OSGetSystemTime() - tBootStart));
}

static inline void removeUselessFiles() {
    //don't load a bunch of extra unneeded files at boot.
    WRITE_NOP(0x8004929c); //various romlist.zlb files
    //(not sure if those are totally unused, or just will be loaded
    //again when needed, but the game seems to do fine.)

    //note, many of these are used by individual maps, but
    //the copies in the disc root aren't used and are loaded
    //at boot, which just wastes memory and time.
    //there are several more unused entries but they're for
    //files that don't exist anyway.
    static const u8 entries[] = { //XXX verify these are all unused.
        0x0C,       //LACTIONS.bin (read, but nothing done)
        0x10,       //FONTS.bin (old, unused)
        0x13, 0x14, //GAMETEXT.bin, .tab (old unused ver)
        0x18, 0x19, //SCREENS.bin, .tab
        0x1A, 0x1B, //VOXMAP.tab, .bin
        0x20, 0x21, //TEX1.bin, .tab
        0x22,       //TEXTABLE.bin (XXX used?)
        0x23, 0x24, //TEX0.bin, .tab
        0x27,       //TRKBLK.tab
        0x28, 0x29, //HITS.bin, .tab
        0x2A, 0x2B, //MODELS.tab, .bin
        0x2C,       //MODELIND.bin
        0x2F, 0x30, //ANIM.TAB, .BIN
        //this one the game seems to work without but generates
        //a bunch of errors about reading outside of a file
        //so it must be used somewhere...
        //0x34,     //WEAPONDA.bin
        0x35, 0x36, //VOXOBJ.tab, .bin
        0x39, 0x3A, //SAVEGAME.bin, .tab
        //these two, the game works without but is constantly
        //trying to reload them...
        //0x40,     //OBJEVENT.bin
        //0x41,     //OBJHITS.bin
        0x43,       //DLLS.tab (.bin doesn't exist)
        //some duplicate entries too
        0x45, 0x46, //MODELS.tab, .bin
        0x49, 0x4A, //ANIM.TAB, .BIN
        0x4B, 0x4C, //TEX1.bin, .tab
        0x4D, 0x4E, //TEX0.bin, .tab
        0x53, 0x54, //VOXMAP.tab, .bin
        0x55, 0x56, //ANIMCURV.bin, .tab
        0xFF}; //end
    for(int i=0; entries[i] != 0xFF; i++) {
        //change some jump table entries to a nearby blr.
        WRITE32(0x802cc534 + entries[i] * 4, 0x80049500);
    }
    // (boot time: seconds without "speed up disc transfer rate" in Dolphin)
    // (fast boot: seconds with "speed up disc transfer rate")
    // boot time | fast boot | bytes used | blocks used | comment
    // 17.492796 |  1.833884 |    3615576 |         558 | with extra files
    // 13.855829 |  1.783834 |    3615672 |         509 | without
    //  3.636967 |  0.050050 |        -96 |          49 | saved
    // memory stats here are strange, since the byte count is near identical
    // (and actually lower with the extras) but the block count isn't.
    // possibly a bug in memory measurement; the game's own stats
    // are wrong for the first heap, so, shrug

    /* for(int i=-100; i<=100; i++) {
        float (*func)(float) = (float (*)(float))0x80294204;
        float r = func(((float)i) / 100.0f);
        OSReport("%f => %f\n", ((float)i) / 100.0f, r);
    } */
}

void initBootHacks() {
    hookBranch(0x80021320, initEndHook, 1);
    removeUselessFiles();
}
