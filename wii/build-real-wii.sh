#!/bin/sh
# Build mod and copy to SD card.
SD_MOUNT_PATH=/run/media/rena/WII
#SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
DISCROOT=/home/rena/projects/sfa/DATA/
NEWDOL=$DISCROOT/sys/main.dol
INSTALL_PATH=$SD_MOUNT_PATH/apps/SFA

if [ ! -e $SD_MOUNT_PATH/apps ]; then
    echo "SD not mounted!"
    exit 1
fi

mkdir -p $INSTALL_PATH/files
mkdir -p $INSTALL_PATH/sys

#echo "Build boot..."
#pushd boot
#    make V=1
#    ok=$?
#popd
#if [ $ok -ne 0 ]; then exit $ok; fi

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

echo "Combine..."
./make-loadable-dol.py $NEWDOL \
    main/main.dol \
    main/build/main.elf.map \
    app/boot.dol
ok=$?
if [ $ok -ne 0 ]; then exit $ok; fi

echo "Install..."
cp -r app/* $INSTALL_PATH
cp $DISCROOT/sys/* $INSTALL_PATH/sys/
cp -ru $DISCROOT/files/* $INSTALL_PATH/files/

umount $SD_MOUNT_PATH
