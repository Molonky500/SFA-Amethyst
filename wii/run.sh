#!/bin/sh
#SD_MOUNT_PATH=/run/media/rena/WHEE
SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
pushd boot
make V=1
ok=$?
popd
if [ $ok -ne 0 ]; then exit $ok; fi
pushd main
make V=1
ok=$?
popd
if [ $ok -ne 0 ]; then exit $ok; fi
mv boot/boot.dol app/boot.dol
mv main/main.dol app/main.dol
if [ -e $SD_MOUNT_PATH/apps ]
then cp -r app/* $SD_MOUNT_PATH/apps/SFA/
else echo "SD not mounted!"
fi
#cp main/build/main.elf.map ???
dolphin-emu -d ./app/boot.dol
#dolphin-emu -d ./boot/boot.elf
