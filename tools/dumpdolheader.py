#!/usr/bin/env python3

import sys
import struct

NUM_TEXT_SECTIONS = 7
NUM_DATA_SECTIONS = 11

def readFmt(file, fmt:str, count:int) -> list:
    size = struct.calcsize(fmt) * count
    data = file.read(size)
    return [*struct.unpack('>%d%s' % (count, fmt), data)]

def main(arg:tuple[str]):
    exeName = arg.pop(0)
    path = arg.pop(0)

    with open(path, 'rb') as file:
        textOffsets = readFmt(file, 'I', NUM_TEXT_SECTIONS)
        dataOffsets = readFmt(file, 'I', NUM_DATA_SECTIONS)
        textAddrs   = readFmt(file, 'I', NUM_TEXT_SECTIONS)
        dataAddrs   = readFmt(file, 'I', NUM_DATA_SECTIONS)
        textSizes   = readFmt(file, 'I', NUM_TEXT_SECTIONS)
        dataSizes   = readFmt(file, 'I', NUM_DATA_SECTIONS)
        bssAddr     = readFmt(file, 'I', 1)[0] # grumble
        bssSize     = readFmt(file, 'I', 1)[0] # grumble
        entryPoint  = readFmt(file, 'I', 1)[0] # grumble

    print("Sect## StartAdr  EndAddr StartOfs  EndOffs     Size")
    for i in range(NUM_TEXT_SECTIONS):
        print("text%2d %8x %8x %8x %8x %8x" % (i,
            textAddrs  [i], textAddrs  [i]+textSizes[i],
            textOffsets[i], textOffsets[i]+textSizes[i],
            textSizes[i]))
    for i in range(NUM_DATA_SECTIONS):
        print("data%2d %8x %8x %8x %8x %8x" % (i,
            dataAddrs  [i], dataAddrs  [i]+dataSizes[i],
            dataOffsets[i], dataOffsets[i]+dataSizes[i],
            dataSizes[i]))
    print("bss    %8x %8x -------- -------- %8x" % (
        bssAddr, bssAddr+bssSize, bssSize))
    print("entry  %8x" % entryPoint)

if __name__ == '__main__':
    main(sys.argv)
