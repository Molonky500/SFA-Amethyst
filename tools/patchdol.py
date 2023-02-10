#!/usr/bin/env python3
"""Inject boot.bin into SFA main.dol."""
from typing import BinaryIO
import os
import sys
import struct

BIN_INJECT_ADDR = 0x803fa480
BIN_SIZE_ADDR   = 0x80240644
JUMP_ADDR       = 0x80020E98
RVL_GLOBALS_END = 0x800031A0
DOL_NUM_TEXT_SECTIONS =  7
DOL_NUM_DATA_SECTIONS = 11
DOL_NUM_SECTIONS = DOL_NUM_TEXT_SECTIONS + DOL_NUM_DATA_SECTIONS

dolHeaderStruct = ('>'
     '7I' # text offsets
    '11I' # data offsets
     '7I' # text addrs
    '11I' # data addrs
     '7I' # text sizes
    '11I' # data sizes
     '3I' # bssAddr, bssSize, entryPoint
)

def readDolHeader(file:BinaryIO) -> dict:
    """Read DOL header from file.

    :param file: File to read from.
    :returns: Dict of header info.
    :note: Reads from current file position.
    """
    data = file.read(struct.calcsize(dolHeaderStruct))
    data = struct.unpack(dolHeaderStruct, data)
    result = {
        'text': [],
        'data': [],
        'bss':  [
            {'addr':data[-3], 'offset':None, 'size':data[-2]},
        ],
        'entry': data[-1],
    }
    for i in range(DOL_NUM_TEXT_SECTIONS):
        offs = data[i]
        addr = data[i+DOL_NUM_SECTIONS]
        size = data[i+(DOL_NUM_SECTIONS*2)]
        result['text'].append({
            'addr':   addr,
            'offset': offs,
            'size':   size,
        })
    for i in range(DOL_NUM_DATA_SECTIONS):
        offs = data[i+DOL_NUM_TEXT_SECTIONS]
        addr = data[i+DOL_NUM_TEXT_SECTIONS+DOL_NUM_SECTIONS]
        size = data[i+DOL_NUM_TEXT_SECTIONS+(DOL_NUM_SECTIONS*2)]
        result['data'].append({
            'addr':   addr,
            'offset': offs,
            'size':   size,
        })
    return result

def writeDolHeader(header:dict, file:BinaryIO) -> None:
    """Write DOL header to file.

    :param header: Dict of header info.
    :param file: File to write to.
    :note: Writes at current file position.
    """
    items = []
    for i in range(DOL_NUM_TEXT_SECTIONS):
        items.append(header['text'][i]['offset'])
    for i in range(DOL_NUM_DATA_SECTIONS):
        items.append(header['data'][i]['offset'])
    for i in range(DOL_NUM_TEXT_SECTIONS):
        items.append(header['text'][i]['addr'])
    for i in range(DOL_NUM_DATA_SECTIONS):
        items.append(header['data'][i]['addr'])
    for i in range(DOL_NUM_TEXT_SECTIONS):
        items.append(header['text'][i]['size'])
    for i in range(DOL_NUM_DATA_SECTIONS):
        items.append(header['data'][i]['size'])
    bss = header['bss'][0]
    items.append(bss['addr'])
    items.append(bss['size'])
    items.append(header['entry'])
    file.write(struct.pack(dolHeaderStruct, *items))

def printDolHeader(header:dict) -> None:
    """Print DOL header to console.

    :param header: Dict of header info.
    """
    print("Sect## Offset Address  EndAddr      Size")
    def printSection(idx, name, sec):
        print("%s%2d %6X %8X %8X %8X" % (name, idx,
            sec['offset'], sec['addr'],
            sec['addr'] + sec['size'],
            sec['size']))

    for i in range(DOL_NUM_TEXT_SECTIONS):
        printSection(i, 'text', header['text'][i])
    for i in range(DOL_NUM_DATA_SECTIONS):
        printSection(i, 'data', header['data'][i])

    bss = header['bss'][0]
    print("bss    ------ %8X %8X %8X" % (
        bss['addr'],
        bss['addr'] + bss['size'],
        bss['size']))
    print("Entry  ------ %8X -------- --------" % header['entry'])

def addrToSection(addr:int, header:dict) -> dict:
    """Return DOL section that contains specified address.

    :param addr: Address to look up.
    :param header: Dict of header info.
    :returns: Section, or None.
    """
    for sec in header['text'] + header['data']:
        if addr >= sec['addr'] and addr < sec['addr']+sec['size']:
            return sec
    return None

def addrToOffset(addr:int, header:dict) -> int:
    """Return DOL file offset that corresponds to
    specified address.

    :param addr: Address to look up.
    :param header: Dict of header info.
    :returns: Offset, or None.
    """
    sec = addrToSection(addr, header)
    if sec is None: return None
    return (addr - sec['addr']) + sec['offset']

def writeAtAddr(file:BinaryIO, header:dict, addr:int,
fmt:str, *vals:int) -> None:
    """Write data at the offset in the DOL file that
    corresponds to the specified address.

    :param file: File to write to.
    :param header: Dict of header info.
    :param addr: Address to write at.
    :param fmt: struct format string.
    :param vals: Value(s) to write.
    """
    offset = addrToOffset(addr, header)
    if offset is None:
        raise ValueError("Address 0x%08X not found in DOL" % addr)
    file.seek(offset, 0)
    file.write(struct.pack(fmt, *vals))

def makeBranch(fromAddr:int, toAddr:int, isBl:bool=True) -> int:
    """Construct a Gekko branch instruction.

    :param fromAddr: Address of the branch instruction.
    :param toAddr: Address to branch to.
    :param isBl: Whether to make a 'bl' instruction,
        rather than a 'b' instruction.
    :returns: Opcode value.
    """
    dist = toAddr - fromAddr
    if abs(dist) >= 32*1024*1024:
        raise ValueError("Jump is too far (%08X -> %08X)" % (
            fromAddr, toAddr))
    return (dist & 0x03FFFFFC) | (1 if isBl else 0) | 0x48000000

def correctSectionAddrs(header:dict) -> None:
    """Adjust the DOL section headers to not overlap
    the Revolution OS globals.

    :param header: Dict of header info.
    :note: Modifies header.
    :note: This will truncate several instructions from the
        beginning of the program. The entry point must be
        updated to point to a copy of those instructions.
    """
    for i in range(DOL_NUM_TEXT_SECTIONS):
        sec = header['text'][i]
        if sec['size'] > 0:
            if sec['addr'] < RVL_GLOBALS_END:
                diff = RVL_GLOBALS_END - sec['addr']
                sec['addr']   += diff
                sec['offset'] += diff
                sec['size']   -= diff
            #if sec['addr'] < 0x80003400:
            #    # lol HBC refuses to load such things
            #    sec['addr'] = 0x80600000
            # but we don't need to worry about that, because
            # HBC isn't loading *this* DOL.
    # for this use case we know no data/bss section needs
    # to be adjusted, so no point doing it

def main():
    args = list(sys.argv[1:])
    if len(args) < 3:
        print("Usage: patchdol.py dolPath binPath outPath")
        sys.exit(1)

    dolPath = args.pop(0)
    binPath = args.pop(0)
    outPath = args.pop(0)
    inDol   = open(dolPath, 'rb')
    inBin   = open(binPath, 'rb')
    outDol  = open(outPath, 'wb')

    inBin.seek(0, 2)
    binSize = inBin.tell()
    inBin.seek(0, 0)
    pad = binSize % 32
    if pad: binSize += 32 - pad

    inDol.seek(0, 2)
    dolSize = inDol.tell()
    inDol.seek(0, 0)

    header = readDolHeader(inDol)
    #printDolHeader(header)

    if addrToSection(BIN_INJECT_ADDR, header) is not None:
        print("DOL is already patched")
        sys.exit(1)

    # inject addr isn't in the DOL, so add it.
    for i in range(DOL_NUM_TEXT_SECTIONS):
        sec = header['text'][i]
        if sec['size'] == 0:
            sec['addr']   = BIN_INJECT_ADDR
            sec['size']   = binSize
            sec['offset'] = dolSize
            break

    # also change the entry point
    header['entry'] = BIN_INJECT_ADDR

    # copy input DOL to output
    inDol.seek(0, 0)
    outDol.write(inDol.read())

    # append the patch
    outDol.write(inBin.read())

    # update the arena address
    newArena = BIN_INJECT_ADDR + binSize
    outDol.seek(addrToOffset(BIN_SIZE_ADDR, header), 0)
    outDol.write(struct.pack('>II',
        0x3C600000 | (newArena >> 16), # lis r3, xxxx
        0x60630000 | (newArena & 0xFFFF) # ori r3, r3, xxxx
    ))

    # and don't overwrite our new arena
    writeAtAddr(outDol, header, 0x8024068c, '>I', 0x60000000)

    # don't call __check_pad3 since it's no longer there.
    writeAtAddr(outDol, header, 0x80003260, '>I', 0x60000000)

    # patch init to jump to the injected code
    # not needed since we changed the entry point
    #outDol.seek(addrToOffset(JUMP_ADDR, header), 0)
    #outDol.write(struct.pack('>I',
    #    makeBranch(JUMP_ADDR, BIN_INJECT_ADDR)))

    # change the sections to not overlap the RVLOS globals
    correctSectionAddrs(header)

    # go back and write the updated header
    outDol.seek(0, 0)
    printDolHeader(header)
    writeDolHeader(header, outDol)

    inDol.close()
    inBin.close()
    outDol.close()


if __name__ == '__main__':
    main()
