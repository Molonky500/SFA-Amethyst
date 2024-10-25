#!/bin/sh
DOLPHIN_SD_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync/apps
make || exit $?
mv patcher.dol patcher/boot.dol
if [ "$1" == "dolphin" ]; then
    cp -r patcher $DOLPHIN_SD_PATH
    dolphin-emu $DOLPHIN_SD_PATH/patcher/boot.dol
else
    zip -r patcher.zip patcher
    /opt/devkitpro/tools/bin/wiiload patcher.zip
fi
