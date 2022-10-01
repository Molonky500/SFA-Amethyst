#!/usr/bin/env python3
"""Create the necessary files for the game to be
recognized as a Wii game.
"""
import struct
import sys
import os
import os.path
import enum

GCN_DISC_MAGIC = 0xC2339F3D
WII_DISC_MAGIC = 0x5D1C9EA3

class RegionCode(enum.IntEnum):
    JAPAN = 0
    USA   = 1
    PAL   = 2
    KOREA = 4

class SignatureType(enum.IntEnum):
    RSA4096 = 0x00010000
    RSA2048 = 0x00010001
    ECC     = 0x00010002

class PublicKeyType(enum.IntEnum):
    RSA4096 = 0
    RSA2048 = 1
    ECC     = 2

def makeDiscHeaderBin(path,
gameId="GSAE01", title="Star Fox Adventures",
discNo=0, version=0, streaming=True, streamBufSize=0):
    """Generate disc/header.bin."""
    # note: streamBufSize=0 uses default size of 10.
    # note: Dolphin writes 256 bytes for header.bin,
    # but Wiibrew says it should be 1024 bytes.
    # the extra bytes are padding anyway...
    data = struct.pack('>6sBBBB14xII64sBB926x',
        bytes(gameId, 'utf-8'), discNo, version,
        1 if streaming else 0, streamBufSize,
        WII_DISC_MAGIC, GCN_DISC_MAGIC,
        bytes(title, 'utf-8'),
        1, # no hash verification
        1, # no encryption
    )
    # GC discs have 0x40 more bytes here (see yagcd ch13)
    # describing things like where to find the executable,
    # FST, etc. Wii discs don't because they use partitions.
    with open(path, 'wb') as file:
        file.write(data)


def makeDiscRegionBin(path,
region=RegionCode.USA):
    """Generate disc/region.bin."""
    data = struct.pack('>I12x10B6x',
        int(region),
        # these bytes define age ratings but their
        # meaning isn't documented so I'm copying
        # from Brawl. 0x80 probably means unrated
        0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80,
    )
    with open(path, 'wb') as file:
        file.write(data)

def makeTmdBin(path):
    """Generate tmd.bin."""
    header = struct.pack('>I256x60x64sBBBBIIIIIHHH16x12x12x18xIHHHH',
        SignatureType.RSA2048,
        # 256 bytes signature (treated as padding here)
        b"Root-CA00000000-CP00000000", # certificate issuer
        0, # Version
        0, # ca_crl_version
        0, # signer_crl_version
        0, # isVwii
        1, 0x24, # system version (copied from Brawl) (seems which IOS to use)
        0x00010000, 0x47534145, # title ID (game, "GSAE")
        1, # title type
        0x3031, # group ID "01"
        0, # unused
        3, # region (all)
        # ratings (zero in Brawl)
        # reserved
        # IPC mask
        # reserved
        0xFFFFFFFF, # access rights (gimme everything)
        0, # title version
        1, # number of contents
        0, # boot index
        0, # minor version
    )
    # content metadata
    cmd = struct.pack('>IHHQ20x',
        0, # content ID
        0, # index
        1, # type (normal) (Brawl has 3 here?)
        int(1.5 * (1024**3)), # size (1.5 GB)
        # SHA1 hash (do we need to fill this in?)
        # obviously real Wii would require it but a real
        # Wii would reject because this isn't signed
        # with Nintendo's private key anyway.
    )
    # some certs come here...
    # Dolphin considers this invalid but boots it anyway...
    # Brawl doesn't have a key here!?
    cert = struct.pack('>I256x32sI64s256x',
        SignatureType.RSA2048, # sig_type
        # sig
        b"Root-CA00000000-CP00000000", # issuer
        PublicKeyType.RSA2048, # key_type
        b"my butt", # name
        # public key (256 bytes for RSA2048)
    )
    with open(path, 'wb') as file:
        file.write(header+cmd+cert)


def main():
    args = list(sys.argv[1:])
    if len(args) < 1:
        print("Usage: %s discpath" % sys.argv[0])
        return
    inPath = args.pop(0)

    pDisc = os.path.join(inPath, 'disc')
    os.makedirs(pDisc, exist_ok=True)

    makeDiscHeaderBin(os.path.join(pDisc, 'header.bin'))
    makeDiscRegionBin(os.path.join(pDisc, 'region.bin'))
    makeTmdBin(os.path.join(inPath, 'tmd.bin'))

    # stick magic in boot.bin
    bootBin = os.path.join(inPath, 'sys', 'boot.bin')
    with open(bootBin, 'r+b') as file:
        file.seek(0x18)
        file.write(struct.pack('>I', WII_DISC_MAGIC))

    # the executable also needs to contain 0x5f617267
    # somewhere in its .text section.

if __name__ == '__main__':
    main()
