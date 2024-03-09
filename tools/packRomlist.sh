#!/bin/bash
# packRomlist.sh animtest.xml game_root_dir
XMLPATH=../../misc/data/U0/
TEMP=/tmp
MAPNAME=${1%%.*} # remove extensions
TMPPATH=$TEMP/$(basename $MAPNAME).romlist.bin
#echo "MAPNAME=$MAPNAME"
#echo "Packing to $2/$(basename $MAPNAME).romlist.zlb"
./romlist.py pack $XMLPATH $1 $TMPPATH
./zlb.py packzlb $TMPPATH $2/$(basename $MAPNAME).romlist.zlb
./romlist.py updateSize $XMLPATH $TMPPATH $2
