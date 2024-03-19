#!/bin/sh

BUILD_PATH=./build
DISCROOT=/home/rena/projects/sfa/DATA/
NEWDOL=$DISCROOT/sys/main.dol
INSTALL_PATH=$BUILD_PATH/SFA
REAL_SD_PATH=/run/media/rena/WII

# for Dolphin
SD_MOUNT_PATH=/home/rena/.local/share/dolphin-emu/Load/WiiSDSync

TARGET=$1
if [ "$TARGET" == "" ]; then TARGET='build'; fi
if [ "$TARGET" == "clean" ]; then
    pushd libfat
        make ogc-clean
    popd
    pushd main
        make clean
    popd
    pushd ..
        ./build.sh clean
    popd
    exit
fi

mkdir -p $INSTALL_PATH/sys

echo "Build libfat..."
pushd libfat
    make wii-release && cp libogc/lib/wii/libfat.a ../main/lib/
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

echo "Combine..."
./make-loadable-dol.py $NEWDOL \
    main/main.dol \
    main/build/main.elf.map \
    app/boot.dol
ok=$?
if [ $ok -ne 0 ]; then exit $ok; fi

if [ "$TARGET" == "build" ]; then
    # Build but don't run
    exit

elif [ "$TARGET" == "dolphin" ]; then
    # Build and run in Dolphin

    if [ -e $SD_MOUNT_PATH/apps ]
    then cp -r app/* $SD_MOUNT_PATH/apps/SFA/
    else
        echo "SD not mounted!"
        exit 1
    fi
    cp -ru $DISCROOT/* $SD_MOUNT_PATH/apps/SFA/files/
    cp -vrf ../patchfiles/* $SD_MOUNT_PATH/apps/SFA/files/
    # this variable runs Dolphin on the primary AMD GPU,
    # instead of the weaker embedded one.
    DRI_PRIME=1 dolphin-emu -d ./app/boot.dol

elif [ "$TARGET" == "realsd" ]; then
    # Build and install to SD card for real Wii

    if [ -e $REAL_SD_PATH/apps ]
    then cp -r app/* $REAL_SD_PATH/apps/SFA/
    else
        echo "SD not mounted!"
        exit 1
    fi
    echo "Copying files..."
    cp -vru $DISCROOT/* $REAL_SD_PATH/apps/SFA/files/
    cp -vruf ../patchfiles/* $REAL_SD_PATH/apps/SFA/files/
    umount $REAL_SD_PATH

elif [ "$TARGET" == "real" ]; then
    # Build and send via wiiload

    echo "Zip..."
    cp -r app/* $INSTALL_PATH
    cp $DISCROOT/sys/* $INSTALL_PATH/sys/
    rm $INSTALL_PATH/sys/main.dol.orig
    #cp -ru $DISCROOT/files/* $INSTALL_PATH/files/

    echo "Upload..."
    pushd $BUILD_PATH
        zip -r sfa.zip ./SFA
    popd
    /opt/devkitpro/tools/bin/wiiload $BUILD_PATH/sfa.zip
fi
