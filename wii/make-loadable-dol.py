#!/usr/bin/env python3
"""
Combine the two DOLs into one that HBC will load.

This script appends dol2 to dol1 and updates the section
headers to get dol2 loaded into RAM. It assumes dol1 knows
that it needs to do something with dol2. The result is
written to outPath; the input files are not modified.

This script also assumes dol2's load addresses don't
overlap dol1's in any way that will break things.
"""
import sys
import struct

NUM_TEXT_SECTIONS = 7
NUM_DATA_SECTIONS = 11

def showUsage(exeName:str):
    print(f"Usage: {exeName} dol1 dol2 outPath loadAddr\n", __doc__)
    sys.exit(1)

def readFmt(file, fmt:str, count:int) -> list:
    size = struct.calcsize(fmt) * count
    data = file.read(size)
    return [*struct.unpack('>%d%s' % (count, fmt), data)]

def main(arg:tuple[str]):
    arg = [*arg[0:]] # make mutable
    exeName = arg.pop(0)
    if len(arg) < 4: showUsage(exeName)

    # get the paths
    inPath1  = arg.pop(0)
    inPath2  = arg.pop(0)
    outPath  = arg.pop(0)
    loadAddr = arg.pop(0)
    try: loadAddr = int(loadAddr, 0)
    except: showUsage(exeName)

    # open the files
    outFile = open(outPath, 'w+b')
    inFile1 = open(inPath1, 'rb')
    inFile2 = open(inPath2, 'rb')

    # write dol1 + dol2 to out
    outFile.write(inFile1.read())
    offs = outFile.tell()
    outFile.write(inFile2.read())
    size = outFile.tell() - offs

    # go back and read the sections
    outFile.seek(0)
    textOffsets = readFmt(outFile, 'I', NUM_TEXT_SECTIONS)
    dataOffsets = readFmt(outFile, 'I', NUM_DATA_SECTIONS)
    textAddrs   = readFmt(outFile, 'I', NUM_TEXT_SECTIONS)
    dataAddrs   = readFmt(outFile, 'I', NUM_DATA_SECTIONS)
    textSizes   = readFmt(outFile, 'I', NUM_TEXT_SECTIONS)
    dataSizes   = readFmt(outFile, 'I', NUM_DATA_SECTIONS)

    # find a free data section
    secIdx = None
    for i in range(NUM_DATA_SECTIONS):
        if dataAddrs[i] == 0:
            secIdx = i
            break
    if secIdx is None:
        raise "No free data sections"

    # add the new section
    dataOffsets[secIdx] = offs
    dataAddrs  [secIdx] = loadAddr
    dataSizes  [secIdx] = size
    outFile.seek(0)
    outFile.write(struct.pack('>%dI' % NUM_TEXT_SECTIONS, *textOffsets))
    outFile.write(struct.pack('>%dI' % NUM_DATA_SECTIONS, *dataOffsets))
    outFile.write(struct.pack('>%dI' % NUM_TEXT_SECTIONS, *textAddrs))
    outFile.write(struct.pack('>%dI' % NUM_DATA_SECTIONS, *dataAddrs))
    outFile.write(struct.pack('>%dI' % NUM_TEXT_SECTIONS, *textSizes))
    outFile.write(struct.pack('>%dI' % NUM_DATA_SECTIONS, *dataSizes))

    inFile1.close()
    inFile2.close()
    outFile.close()

if __name__ == '__main__':
    main(sys.argv)
