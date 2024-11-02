# Drive commands
DICMDBUF0 |DICMDBUF1 |DICMDBUF2 |DIMAR|DILENGTH|DIIMMBUF|DICR|Description
----------|----------|----------|-----|--------|--------|----|-----------
0x12000000|0x00000000|0x00000020|ret: Drive Info|0x00000020|-|DMA read|Inquiry
0xa8000000|Data Position >> 2|Data Length|ret: Sector Data|Data Length|-|DMA read|read Sector
0xa8000040|0x00000000|0x00000020|ret: Disc ID|0x00000020|-|DMA read|read Disc ID/Init Drive
0xa8000080|?|?|?|?|?|?|?
0xa80000C0|?|?|?|?|?|?|?
0xab000000|Position >> 2|-|-|-|-|imm (read)|seek
0xe0000000|-|-|-|-|ret: Error Code|imm read|request error Status
0xe1??0000|Stream Position >> 2|Stream Length|-|-|-|imm read|play Audio Stream (?)
0xe2??0000|-|-|-|-|ret: Status (?)|imm read|request Audio Status
0xe3000000|-|-|-|-|-|imm (read)|stop Motor
0xe4000000|-|-|-|-|-|imm (read)|DVD Audio disable
0xe4010000|-|-|-|-|-|imm (read)|DVD Audio enable

## Debug commands
DICMDBUF0 |DICMDBUF1 |DICMDBUF2 |DIMAR|DILENGTH|DIIMMBUF|DICR|Description
----------|----------|----------|-----|--------|--------|----|-----------
0xfe00????|?|?|?|?|?|?|?
0xfe010000|offset|0x00010000|-|-|ret: 32bit value|imm (read)|read memory
0xfe010100|offset|0x00010000|-|-|32bit value|imm (write)|write cache
0xfe018000|offset|0xff000000|-|-|ret: 32bit value|imm (read)|read cache
0xfe018100|offset|0xff000000|-|-|32bit value|imm (write)|write cache
0xfe100000|?|?|-|-|-|imm (read)|?
0xfe110000 (*)|-|-|-|-|-|imm (read)|stop drive
0xfe110100 (*)|-|-|-|-|-|imm (read)|start drive
0xfe114000 (*)|-|-|-|-|-|imm (read)|accept copy
0xfe118000 (*)|-|-|-|-|-|imm (read)|do disc-check
0xfe120000|24bit address|0x66756e63|-|-|-|imm (read)|jsr to address ’func’
0xff004456|0x442d4741|0x4d450300|-|-|-|imm (read)|unlock 2 ’DVD-GAME’
0xff014d41|0x54534849|0x5441024f|-|-|-|imm (read)|unlock 1 ’MATSHITA’

(*) commands can be ORed to perform several actions at once

# Error codes
## high byte = error code 1
* 0x01: lid open
* 0x02: no disc/disc changed
* 0x03: no disc
* 0x04: motor off
* 0x05: disc not init/ID not read

## low bytes = error code 2
* 0x020400: Motor stopped
* 0x020401: Disk ID not read
* 0x023A00: Medium not present / Cover opened
* 0x030200: No Seek complete
* 0x031100: UnRecoverd read error
* 0x040800: Transfer protocol error
* 0x052000: Invalid command operation code
* 0x052001: Audio Buffer not set
* 0x052100: Logical block address out of range
* 0x052400: Invalid Field in command packet
* 0x052401: Invalid audio command
* 0x052402: Conﬁguration out of permitted period
* 0x056300: End of user area encountered on this track
* 0x062800: Medium may have changed
* 0x0B5A01: Operator medium removal request
