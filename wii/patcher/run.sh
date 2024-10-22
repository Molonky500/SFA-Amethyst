#!/bin/sh
make || exit $?
mv patcher.dol patcher/boot.dol
if [ "$1" == "dolphin" ]; then
    dolphin-emu patcher/boot.dol
else
    zip -r patcher.zip patcher
    /opt/devkitpro/tools/bin/wiiload patcher.zip
fi
