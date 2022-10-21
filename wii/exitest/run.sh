#!/bin/sh
make || exit $?
mv exitest.dol exitest/boot.dol
zip -r exitest.zip exitest
/opt/devkitpro/tools/bin/wiiload exitest.zip
