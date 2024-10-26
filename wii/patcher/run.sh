#!/bin/sh
DOLPHIN_SD_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync/apps
make || exit $?
mv patcher.dol patcher/boot.dol

# convert resources.
# XXX this shouldn't be here...
find res -name \*.png -exec sh -c '../../tools/texconv/__main__.py pack RGBA32 1 "{}" "patcher/${1%.*}.tex"' sh {} \;

if [ "$1" == "dolphin" ]; then
    cp -r patcher $DOLPHIN_SD_PATH
    dolphin-emu $DOLPHIN_SD_PATH/patcher/boot.dol
elif [ "$1" == "dummy" ]; then
    # do nothing
    true # must be a statement here
else
    zip -r patcher.zip patcher
    /opt/devkitpro/tools/bin/wiiload patcher.zip
fi
