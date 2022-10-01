#!/bin/sh
DISCROOT=/home/rena/projects/sfa/DATA/
#SD_MOUNT_PATH=/run/media/rena/WHEE
SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
./build.sh install
ok=$?
if [ $ok -ne 0 ]; then exit $ok; fi
cp $DISCROOT/sys/main.dol ./wii/app/boot.dol
if [ -e $SD_MOUNT_PATH/apps ]
then cp -r wii/app/* $SD_MOUNT_PATH/apps/SFA/
else
    echo "SD not mounted!"
    exit 1
fi
dolphin-emu -d -n 000100014f484243
