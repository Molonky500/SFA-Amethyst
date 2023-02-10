#!/bin/sh
# Build mod and copy to SD card.
SD_MOUNT_PATH=/run/media/rena/WII
#SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
DISCROOT=/home/rena/projects/sfa/DATA/

if [ ! -e $SD_MOUNT_PATH/apps ]; then
    echo "SD not mounted!"
    exit 1
fi

echo "Build boot..."
pushd boot
    make V=1
    ok=$?
popd
if [ $ok -ne 0 ]; then exit $ok; fi

echo "Build main..."
pushd main
    make V=1
    ok=$?
popd
if [ $ok -ne 0 ]; then exit $ok; fi

echo "Build game..."
pushd ..
    ./build.sh install
    ok=$?
popd
if [ $ok -ne 0 ]; then exit $ok; fi

echo "Install..."
mv boot/boot.dol app/boot.dol
mv main/main.dol app/main.dol
cp -r app/* $SD_MOUNT_PATH/apps/SFA/
cp $DISCROOT/sys/* $SD_MOUNT_PATH/apps/SFA/sys/

umount $SD_MOUNT_PATH
