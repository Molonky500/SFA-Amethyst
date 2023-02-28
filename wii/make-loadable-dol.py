#!/usr/bin/env python3
"""
Patch the game's main.dol to inject the loader and
move sections so that HBC will load it.

The loader DOL needs to contain a symbol `_start_hack`;
the entry point will be changed to point to it. This
should be a function that will move sections back to
the correct places, load the appended loader DOL into
the right places, and jump to it.

This hack works by finding where the symbol ends up in
RAM before the loader DOL is actually "loaded" and
pointing the entry point there.
"""
import re
import sys
import struct

NUM_TEXT_SECTIONS = 7
NUM_DATA_SECTIONS = 11
NEW_ENTRY_SYMBOL  = '_start_hack' # change entry point to this
NEW_LOAD_ADDR     = 0x80510000 # where to append sections
MOVE_SEC_ADDR     = 0x80500000 # where to move text0
# we save an instruction by having the low 16 bits be zero.
# it doesn't waste memory because it gets moved from here
# anyway. not that this really matters...

def showUsage(exeName:str):
    print(f"Usage: {exeName} gameMain.dol loader.dol loader.map out.dol\n", __doc__)
    sys.exit(1)

def readFmt(file, fmt:str, count:int) -> list:
    """Read struct format array.

    :param file: File to read from.
    :param fmt: Format string.
    :param count: Number of instances.
    :returns: List of items read from file.
    """
    size = struct.calcsize(fmt) * count
    data = file.read(size)
    return [*struct.unpack('>%d%s' % (count, fmt), data)]

re_mapLine = re.compile(r'^\s+(0x[0-9a-fA-F]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)$')
def readMap(path:str) -> dict[str,int]:
    """Read .map file.

    :param path: Path to file.
    :returns: dict of symbol name => address.
    """
    print("Reading map:", path)
    result = {}
    with open(path, 'rt') as file:
        for line in file:
            match = re_mapLine.match(line)
            if match is None: continue
            addr, name = int(match.group(1), 0), match.group(2)
            if addr == 0: continue
            if name in result:
                print("Duplicate symbol '%s' (addr 0x%X, 0x%X)" % (
                    name, addr, result[name]))
            result[name] = addr
    return result

def parseDolHeader(data:bytes) -> dict:
    offsets = struct.unpack_from('>%dI%dI%dI%dI%dI%dIIII' % (
        NUM_TEXT_SECTIONS, NUM_DATA_SECTIONS,
        NUM_TEXT_SECTIONS, NUM_DATA_SECTIONS,
        NUM_TEXT_SECTIONS, NUM_DATA_SECTIONS), data)
    result = {
        'bss': {
            'address': offsets[-3],
            'size':    offsets[-2],
        },
        'entry': offsets[-1],
    }
    nSections = NUM_TEXT_SECTIONS + NUM_DATA_SECTIONS
    for i in range(NUM_TEXT_SECTIONS):
        result['text%d' % i] = {
            'offset':  offsets[i],
            'address': offsets[i + nSections],
            'size':    offsets[i + (nSections * 2)],
        }
    for i in range(NUM_DATA_SECTIONS):
        result['data%d' % i] = {
            'offset':  offsets[i + NUM_TEXT_SECTIONS],
            'address': offsets[i + NUM_TEXT_SECTIONS + nSections],
            'size':    offsets[i + NUM_TEXT_SECTIONS + (nSections * 2)],
        }
    return result

def printDolHeader(header:dict) -> None:
    print("Sect##  Address Offset   Size")
    for i in range(NUM_TEXT_SECTIONS):
        sec = header['text%d' % i]
        print("text%2d %8x %6x %6x" % (i,
            sec['address'], sec['offset'], sec['size']))
    for i in range(NUM_DATA_SECTIONS):
        sec = header['data%d' % i]
        print("data%2d %8x %6x %6x" % (i,
            sec['address'], sec['offset'], sec['size']))
    print("bss    %8x ------ %6x" % (
        header['bss']['address'], header['bss']['size']))
    print("entry  %8x" % header['entry'])

def buildDolHeader(header:dict) -> bytes:
    result = []
    for i in range(NUM_TEXT_SECTIONS):
        result.append(struct.pack('>I', header['text%d'%i]['offset']))
    for i in range(NUM_DATA_SECTIONS):
        result.append(struct.pack('>I', header['data%d'%i]['offset']))
    for i in range(NUM_TEXT_SECTIONS):
        result.append(struct.pack('>I', header['text%d'%i]['address']))
    for i in range(NUM_DATA_SECTIONS):
        result.append(struct.pack('>I', header['data%d'%i]['address']))
    for i in range(NUM_TEXT_SECTIONS):
        result.append(struct.pack('>I', header['text%d'%i]['size']))
    for i in range(NUM_DATA_SECTIONS):
        result.append(struct.pack('>I', header['data%d'%i]['size']))
    result.append(struct.pack('>3I', header['bss']['address'],
        header['bss']['size'], header['entry']))
    return b''.join(result)

def main(arg:tuple[str]):
    arg = [*arg[0:]] # make mutable
    exeName = arg.pop(0)
    if len(arg) < 4: showUsage(exeName)

    # get the paths
    origPath   = arg.pop(0)
    loaderPath = arg.pop(0)
    mapPath    = arg.pop(0)
    outPath    = arg.pop(0)
    print("origPath:", origPath)
    print("loaderPath:", loaderPath)
    print("mapPath:", mapPath)
    print("outPath:", outPath)

    # open the files
    mapData    = readMap(mapPath)
    outFile    = open(outPath,    'w+b')
    origFile   = open(origPath,   'rb')
    loaderFile = open(loaderPath, 'rb')

    # write original DOL to out
    outFile.write(origFile.read())
    loaderOffs = outFile.tell() # note the file offset

    # write loader DOL
    loader = loaderFile.read()
    outFile.write(loader)
    loaderSize = outFile.tell() - loaderOffs

    # find the new entry point symbol and compute
    # where it will end up before loading
    entryAddr = mapData.get(NEW_ENTRY_SYMBOL, None)
    if entryAddr is None:
        raise ValueError("Entry symbol '%s' not found in %s" % (
            NEW_ENTRY_SYMBOL, loaderPath))
    loaderSections = parseDolHeader(loader)
    for name, sec in loaderSections.items():
        if name in ('entry', 'bss'): continue
        aStart = sec['address']
        aEnd   = sec['address'] + sec['size']
        if entryAddr >= aStart and entryAddr < aEnd:
            # entry point is in this section
            print("Entry symbol in section %s (%08X-%08X) at %08X" % (
                name, aStart, aEnd, entryAddr))
            entryAddr = (entryAddr - aStart) + NEW_LOAD_ADDR + sec['offset']
            print(" -> %08X" % entryAddr)
            break
    print("Entry point: 0x%08X -> 0x%08X" % (entryAddr,
        loaderSections['entry']))

    # go back and read the sections
    outFile.seek(0)
    sections = parseDolHeader(outFile.read(
        (((NUM_TEXT_SECTIONS + NUM_DATA_SECTIONS) * 3) + 3) * 4))
    print("Original sections:")
    printDolHeader(sections)

    # move sections that are too low for HBC
    # (this is only text0 and I'm lazy so this won't work for
    # data sections or more than one section)
    for i in range(NUM_TEXT_SECTIONS):
        sec = sections['text%d' % i]
        if sec['address'] > 0 and sec['address'] <= 0x80003400:
            print("Moving text%d from 0x%X to 0x%X" % (i,
                sec['address'], MOVE_SEC_ADDR))
            sec['address'] = MOVE_SEC_ADDR

    # find a free text section
    secIdx = None
    for i in range(NUM_TEXT_SECTIONS):
        if sections['text%d' % i]['size'] == 0:
            secIdx = 'text%d' % i
            break
    if secIdx is None:
        raise ValueError("No free text sections")

    # add the new section and update the entry point
    sections[secIdx] = {
        'address': NEW_LOAD_ADDR,
        'size':    loaderSize,
        'offset':  loaderOffs,
    }
    sections['entry'] = entryAddr
    outFile.seek(0)
    outFile.write(buildDolHeader(sections))

    origFile  .close()
    loaderFile.close()
    outFile   .close()

if __name__ == '__main__':
    main(sys.argv)
