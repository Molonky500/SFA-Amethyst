#!/bin/sh
# Build mod and run in Dolphin. (XXX not tested)
#SD_MOUNT_PATH=/run/media/rena/WII
SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync
# XXX move this
DISCROOT=/home/rena/projects/sfa/DATA/files
NEWDOL=$DISCROOT/../sys/main.dol

#pushd boot
#    make V=1
#    ok=$?
#popd
#if [ $ok -ne 0 ]; then exit $ok; fi

pushd main
    make V=1
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

if [ -e $SD_MOUNT_PATH/apps ]
then cp -r app/* $SD_MOUNT_PATH/apps/SFA/
else
    echo "SD not mounted!"
    exit
fi

#cp main/build/main.elf.map ???
if [ "$1" != "--dry-run" ]; then
    # this variable runs Dolphin on the primary AMD GPU,
    # instead of the weaker embedded one.
    DRI_PRIME=1 dolphin-emu -d ./app/boot.dol
    #dolphin-emu -d ./boot/boot.elf
fi
