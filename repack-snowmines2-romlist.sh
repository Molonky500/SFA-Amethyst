#!/bin/sh
./tools/romlist.py pack ./../misc/data/U0/ ~/temp/snowmines2.pretty.xml ~/temp/snowmines2.new.romlist
./tools/zlb.py packzlb ~/temp/snowmines2.new.romlist ./patchfiles/files/snowmines2.romlist.zlb
cp ./patchfiles/files/snowmines2.romlist.zlb ~/.local/share/dolphin-emu/Load/WiiSDSync/apps/SFA/files/
