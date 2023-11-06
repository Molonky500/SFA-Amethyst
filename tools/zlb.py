#!/usr/bin/env python3
"""Compress/decompress ZLB files."""
from typing import BinaryIO
import zlib
import os
import sys
import struct

zlbHeader = ('>'
    '4s' # signature: "ZLB\0"
     'I' # version (always 1)
     'I' # decompressed length
     'I' # compressed length
)
dirHeader = ('>'
    '4s' # signature: "DIR\0" or "DIRn"
     'I' # version (always 1)
     'I' # decompressed length
     'I' # compressed length (same as decompressed length)
)

def decompressZLB(data:bytes) -> bytes:
    """Decompress ZLB."""
    header = struct.unpack_from(zlbHeader, data, 0)
    assert header[0] == b'ZLB\0', "Invalid ZLB header"
    assert header[1] == 1, "Invalid ZLB version"
    decLen, compLen = header[2], header[3]
    data = zlib.decompress(data[16:16+compLen])
    if len(data) != decLen:
        print("ZLB: Unpacked length is %d but should be %d" % (
            len(data), decLen))
    return data

def decompressDIR(data:bytes) -> bytes:
    """Decompress DIR."""
    # this isn't actually compressed, but it's used in places
    # where compression would be used.
    header = struct.unpack_from(zlbHeader, data, 0)
    assert header[0] in (b'DIR\0', b'DIRn'), "Invalid DIR header"
    assert header[1] == 1, "Invalid DIR version"
    decLen, compLen = header[2], header[3]
    assert decLen == compLen, "DIR length mismatch"
    return data[16:16+decLen]

def decompress(data:bytes) -> bytes:
    """Decompress ZLB and related formats."""
    sig = data[0:4]
    if sig == b'ZLB\0': return decompressZLB(data)
    elif sig == b'DIR\0' or sig == 'DIRn': return decompressDIR(data)
    else:
        # XXX support FACEFEED, but what to do with the data before
        # the compressed part?
        raise TypeError("Unsupported file type")

def compressZLB(data:bytes, level:int=9, wbits:int=13) -> bytes:
    """Compress the given data using ZLB format.

    :param data: Data to compress.
    :param level: level parameter for Zlib.
    :param wbits: wbits parameter for Zlib.

    The default level and wbits values are those used by the game,
    but it can handle other values.
    """
    comp  = zlib.compressobj(level=level,wbits=wbits)
    cData = comp.compress(data) + comp.flush()
    return (
        b'ZLB\0' + b'\0\0\0\x01' + # magic, version
        struct.pack('>II', len(data), len(cData)) +
        cData)

def compressDIR(data:bytes, sig:bytes=b'DIR\0') -> bytes:
    """ "Compress" the given data using DIR format.
    This format is not actually compressed, but just wrapped;
    the game uses it in place of compression sometimes.
    """
    return sig + b'\0\0\0\x01' + struct.pack('>II',
        len(data), len(data)) + data

def showUsage() -> None:
    print("Usage: %s MODE INPATH OUTPATH" % sys.argv[0])
    print("MODE is one of: packzlb, packdir, unpack")

def main(args:list[str]) -> None:
    try:
        mode    = args.pop(0)
        inPath  = args.pop(0)
        outPath = args.pop(0)
    except IndexError:
        showUsage()
        return
    if mode == 'packzlb':
        with open(inPath, 'rb') as inFile:
            with open(outPath, 'wb') as outFile:
                outFile.write(compressZLB(inFile.read()))
    elif mode == 'packdir':
        with open(inPath, 'rb') as inFile:
            with open(outPath, 'wb') as outFile:
                outFile.write(compressDIR(inFile.read()))
    elif mode == 'unpack':
        with open(inPath, 'rb') as inFile:
            with open(outPath, 'wb') as outFile:
                outFile.write(decompress(inFile.read()))
    else:
        showUsage()

if __name__ == '__main__':
    main(list(sys.argv[1:]))
