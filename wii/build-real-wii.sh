#!/bin/sh
# Build mod and send via wiiload.
BUILD_PATH=./build
DISCROOT=/home/rena/projects/sfa/DATA/
NEWDOL=$DISCROOT/sys/main.dol
INSTALL_PATH=$BUILD_PATH/SFA

mkdir -p $INSTALL_PATH/sys

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

echo "Zip..."
cp -r app/* $INSTALL_PATH
cp $DISCROOT/sys/* $INSTALL_PATH/sys/
#cp -ru $DISCROOT/files/* $INSTALL_PATH/files/

echo "Upload..."
pushd $BUILD_PATH
zip -r sfa.zip ./SFA
popd
/opt/devkitpro/tools/bin/wiiload $BUILD_PATH/sfa.zip
