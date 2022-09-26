#include "main.h"

static inline void tweakDebugStuff() {
    //attempt to fix heap metrics (doesn't actually work)
    //WRITE32(0x800239ec, 0x38A5FFFC); //subi      r5,r5,0x4
    //WRITE32(0x80023a0c, 0x8003FFF8); //lwz       r0,-0x8(size2)
    //these won't work here because the code has already run.
    //need to directly patch the DOL instead.
    //XXX automate that.
    //WRITE32(0x80023f18, 0x9085FFF8); //stw       size,-0x8(slots)
    //WRITE32(0x80023f1c, 0x9005FFFC); //stw       r0,-0x4(slots)

    WRITE_NOP(0x80119D90); //chapter select only needs Z button
}

static inline void tweakMoveSpeed() {
    //increase climbing speeds
    WRITEFLOAT(0x803E8000, 0.05); //wall climb (up and down)
    WRITE16(0x802A26BA, 0x1B70); //ladder climb (up)
    WRITE16(0x802A26A6, 0x1B70); //ladder climb (down)

    //increase block pushing speeds
    WRITE16(0x8029df1a, 0x19E0); //moving left
    WRITE16(0x8029def6, 0x1A90); //moving right
    WRITE16(0x8029dec6, 0x1A90); //moving back 0.05 -> 0.98
    WRITE16(0x8029de9a, 0x1EC8); //moving forward -0.05 -> -1
}

static inline void tweakArwing() {
    //kill Arwing health alarm. we can turn it back
    //on from the menu if we really want.
    WRITE_NOP(0x8022c2f0);

    //make gold rings restore an entire heart
    WRITE32(0x8022fc50, 0x38800004);
}

static inline void tweakCMenu() {
    //remove useless items from C menu
    //XXX all of these are needed at some point
    //except fuel cells.
    //WRITE16(0x8031b1c0, 0x0096); //water spellstone 1
    //WRITE16(0x8031b1d0, 0x0096); //spellstone ?
    //WRITE16(0x8031b1e0, 0x0096); //spellstone ?
    //WRITE16(0x8031b1f0, 0x0096); //spellstone ?
    //WRITE16(0x8031b360, 0x0096); //fire spellstone 1
    //WRITE16(0x8031b370, 0x0096); //spellstone ?
    //WRITE16(0x8031b380, 0x0096); //fire spellstone 2
    //WRITE16(0x8031b390, 0x0096); //water spellstone 2
    WRITE16(0x8031b3e0, 0x0096); //fuel cells
    //WRITE16(0x8031b440, 0x0096); //give scarabs
    //WRITE16(0x8031b450, 0x0096); //cheat token 0
    //WRITE16(0x8031b460, 0x0096); //cheat token 3
    //WRITE16(0x8031b470, 0x0096); //cheat token 2
    //WRITE16(0x8031b480, 0x0096); //cheat token 6
    //WRITE16(0x8031b490, 0x0096); //cheat token 4
    //WRITE16(0x8031b4a0, 0x0096); //cheat token 7
    //WRITE16(0x8031b4b0, 0x0096); //cheat token 1
    //WRITE16(0x8031b4c0, 0x0096); //cheat token 5
    //WRITE16(0x8031b530, 0x0096); //Open Portal spell
    //WRITE16(0x8031b540, 0x0096); //Staff Booster spell
}

void tweaks_init() {
    tweakDebugStuff();
    tweakMoveSpeed();
    tweakArwing();
    tweakCMenu();

    //disable getLActions() since the file isn't loaded anymore
    WRITE_BLR(getLActions);
}
