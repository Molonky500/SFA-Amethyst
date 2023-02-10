#!/bin/sh
# OLD SCRIPT - use wii/build-real-wii.sh instead.
DISCROOT=/home/rena/projects/sfa/DATA/
SD_MOUNT_PATH=/run/media/rena/WII
#SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
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
cp $DISCROOT/sys/* $SD_MOUNT_PATH/apps/SFA/sys/
#dolphin-emu -d -n 000100014f484243
