#!/bin/sh
make || exit $?
mv iguanatest.dol iguanatest/boot.dol
zip -r iguanatest.zip iguanatest
/opt/devkitpro/tools/bin/wiiload iguanatest.zip
