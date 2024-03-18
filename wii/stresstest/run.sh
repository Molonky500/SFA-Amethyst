#!/bin/sh
make || exit $?
mv stresstest.dol stresstest/boot.dol
zip -r stresstest.zip stresstest
/opt/devkitpro/tools/bin/wiiload stresstest.zip
