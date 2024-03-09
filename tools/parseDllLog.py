#!/usr/bin/env python3
import os
import sys
import struct

"""
typedef struct {
    u16 param;
    u32 time;
    float playerPos[3];
    s8 mapLayer;
    void *ra;
} DllLogEntry;"""

def main(inPath:str) -> None:
    dlls = []
    fmt  = '>HIfffbI'
    size = struct.calcsize(fmt)
    with open(inPath, 'rb') as file:
        while True:
            data = file.read(size)
            if not data: break
            data = struct.unpack(fmt, data)
            dlls.append({
                'param':      data[0],
                'time':       data[1],
                'playerPos': (data[2], data[3], data[4]),
                'mapLayer':   data[5],
                'ra':         data[6],
            })
    for i, dll in enumerate(dlls):
        print("%04X %04X %10d %+10.2f %+10.2f %+10.2f %2d %08X" % (
            i, dll['param'], dll['time'],
            dll['playerPos'][0], dll['playerPos'][1], dll['playerPos'][2],
            dll['mapLayer'], dll['ra']))

if __name__ == '__main__':
    main(*sys.argv[1:])
