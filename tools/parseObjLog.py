#!/usr/bin/env python3
import os
import sys
import struct

def main(inPath:str) -> None:
    entries = []
    fmt  = '>IfffbhbIhBBBBBBfffI32I'
    size = struct.calcsize(fmt)
    with open(inPath, 'rb') as file:
        while True:
            data = file.read(size)
            if not data: break
            data = struct.unpack(fmt, data)
            entries.append({
                'time':          data[0],
                'playerPos':    (data[1], data[2], data[3]),
                'mapLayer':      data[4],
                'mapId':         data[5],
                # 6 is padding
                'flags':         data[7],
                'objType':       data[8],
                'allocatedSize': data[9],
                'mapActs1':      data[10],
                'loadFlags':     data[11],
                'mapActs2':      data[12],
                'bound':         data[13],
                'unk7':          data[14],
                'pos':          (data[15], data[16], data[17]),
                'id':            data[18],
                'params':        data[19:],
            })
    print("Obj#│ Timestamp│   PlayerX    PlayerY    PlayerZ Ly│Mp    Flags Sz A1 LF A2 Bd  ?    ObjectX    ObjectY    ObjectZ UniqueID Params")
    for i, entry in enumerate(entries):
        print("%04X│%10d│%+10.2f %+10.2f %+10.2f %2d│%02X %08X %2d %02X %02X %02X %02X %02X %+10.2f %+10.2f %+10.2f %08X %s" % (i,
            entry['time'], entry['playerPos'][0], entry['playerPos'][1],
            entry['playerPos'][2], entry['mapLayer'],
            entry['mapId'], entry['flags'], entry['allocatedSize'],
            entry['mapActs1'], entry['loadFlags'], entry['mapActs2'],
            entry['bound'], entry['unk7'],
            entry['pos'][0], entry['pos'][1], entry['pos'][2], entry['id'],
            ' '.join(map(lambda n: '%08X' % n, entry['params'])),
        ))

if __name__ == '__main__':
    main(*sys.argv[1:])
