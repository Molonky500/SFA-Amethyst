#!/usr/bin/env python3
import struct

BASE_PATH = '/home/rena/.local/share/dolphin-emu/Load/WiiSDSync/apps/SFA/files/'
with open(BASE_PATH+'test1.bin', 'wb') as file:
    for i in range(4096):
        file.write(struct.pack('>I', i))

with open(BASE_PATH+'test2.bin', 'wb') as file:
    for i in range(16384):
        file.write(struct.pack('>I', i))

with open(BASE_PATH+'test3.bin', 'wb') as file:
    for i in range(65536):
        file.write(struct.pack('>I', i))

with open(BASE_PATH+'test4.bin', 'wb') as file:
    for i in range(131072):
        file.write(struct.pack('>I', i))

with open(BASE_PATH+'test5.bin', 'wb') as file:
    for i in range(262144):
        file.write(struct.pack('>I', i))
