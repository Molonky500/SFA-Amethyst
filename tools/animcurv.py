#!/usr/bin/env python3
from __future__ import annotations
"""Build/unpack animcurv files."""

from typing import BinaryIO, TextIO
from pathlib import Path
import os
import sys
import struct
import enum
import xml.etree.ElementTree as ET

class AnimCurvCommand(enum.IntEnum):
    """Values for the Action Command parameter."""
    SETTIME           = 0x00
    MOVEMODE          = 0x01
    ANIM              = 0x02
    OVERRIDE          = 0x03
    VTXANIM           = 0x04
    SOFTWARE          = 0x05
    SFX               = 0x06
    GROUND_MODE       = 0x07
    TUNE              = 0x08
    ANGLE_MODE        = 0x09
    LOOK_AT           = 0x0A
    BGCMD             = 0x0B
    SPEECH            = 0x0C
    ENVFX             = 0x0D
    STORYBOARD        = 0x0E
    SFX_WITH_DURATION = 0x0F
    UNK27             = 0x27 # found in curve 154 in snowmines2 (BGCMD param?)
    EXTPARAM          = 0x7F
    ENDTIME           = 0xFF

class AnimCurvField(enum.IntEnum):
    """Values for the curve field parameter."""
    HEAD_ROT_Z  = 0x00
    HEAD_ROT_X  = 0x01
    HEAD_ROT_Y  = 0x02
    OPACITY     = 0x03
    TIME_OF_DAY = 0x04 # game ignores this field
    SCALE       = 0x05
    OBJ_ROT_Z   = 0x06
    OBJ_ROT_X   = 0x07
    OBJ_ROT_Y   = 0x08
    ANIM_TIMER  = 0x09
    POINT_SOUND = 0x0A # unsure what this is
    OBJ_POS_Z   = 0x0B
    OBJ_POS_Y   = 0x0C # XXX double-check if Y/X are swapped
    OBJ_POS_X   = 0x0D
    CAMERA_FOV  = 0x0E
    EYE_ROT_X   = 0x0F # left/right
    EYE_ROT_Y   = 0x10 # up/down
    OPEN_MOUTH  = 0x11 # Z rotation
    UNK_SOUND   = 0x12

class AnimCurvAction:
    """One action in a curve."""

    command: AnimCurvCommand = None
    """The command."""

    time: int = None
    """How many frames before advancing to next command."""

    param: int = None
    """The parameter."""

    def __init__(self, command:AnimCurvCommand=None, time:int=None, param:int=None):
        self.command = AnimCurvCommand(command)
        self.time = time
        self.param = param

    @classmethod
    def fromBytes(cls, data:bytes) -> AnimCurv:
        return cls(*struct.unpack('>BBh', data))

class AnimCurvPoint:
    """One point in a curve."""

    y: float = None
    """The Y coordinate."""

    type: int = None
    """The curve type."""

    scale: float = None
    """The scale. (Ignored for types other than 0)"""

    field: AnimCurvField = None
    """Which field this curve belongs to."""

    unk05: int = None
    """High bits of the field value."""

    x: float = None
    """The X coordinate."""

    def __init__(self, y:float, typeAndScale:int, field:AnimCurvField, x:float):
        self.x = x
        self.y = y
        self.type  = typeAndScale & 3
        self.scale = (typeAndScale >> 2) / 16
        self.field = AnimCurvField(field & 0x1F)
        self.unk05 = field >> 5

    @classmethod
    def fromBytes(cls, data:bytes) -> AnimCurvPoint:
        return cls(*struct.unpack('>fBBh', data))

class AnimCurv:
    """One animation curve."""

    signature: str = None
    """Either SEQA or SEQB, or None."""

    actions: list[AnimCurvAction] = None
    """The actions."""

    points: dict[AnimCurvField,list[AnimCurvPoint]] = None
    """The curves."""

    offset: int = None
    """Offset of this curve in the binary, for debugging."""

    @classmethod
    def fromBytes(cls, data:bytes) -> AnimCurv:
        self = cls()
        self.actions = []
        self.points  = {}
        self.signature = struct.unpack_from('4s', data)[0] # grumble
        if self.signature in (b'SEQA', b'SEQB'):
            self.signature = self.signature.decode('utf-8')
            size, nActions = struct.unpack_from('>HH', data[4:])
            data = data[8:]
            #print(f"size={size} nAct={nActions} data={data}")
            for i in range(nActions):
                self.actions.append(AnimCurvAction.fromBytes(data[0:4]))
                data = data[4:]
            while len(data) > 0:
                point = AnimCurvPoint.fromBytes(data[0:8])
                data = data[8:]
                if point.field not in self.points:
                    self.points[point.field] = []
                self.points[point.field].append(point)
        else: # only some commands
            self.signature = None
            for i in range(0, len(data), 4):
                self.actions.append(AnimCurvAction.fromBytes(data[i:i+4]))
        return self

class AnimCurvReader:
    """Parses ANIMCURV binary files."""

    binPath: os.PathLike = None
    """Path to ANIMCURV.bin file."""

    tabPath: os.PathLike = None
    """Path to ANIMCURV.tab file."""

    offsets: list[int] = None
    """Offset of each curve in the file."""

    curves: list[AnimCurv] = None
    """The curves."""

    def __init__(self, binPath:os.PathLike, tabPath:os.PathLike):
        self.binPath = binPath
        self.tabPath = tabPath
        self.offsets = []
        self.curves  = []

    def decode(self) -> None:
        """Decode the files."""
        with open(self.tabPath, 'rb') as file:
            while True:
                data = file.read(4)
                if data is None or len(data) < 4: break
                data = struct.unpack('>I', data)[0] # grumble
                self.offsets.append(data)
                if data == 0xFFFFFFFF: break

        with open(self.binPath, 'rb') as file:
            for i, offs in enumerate(self.offsets[:-1]):
                if not offs & 0x80000000: continue
                oNext = self.offsets[i+1] & 0xFFFFFF
                dlen  = oNext - (offs & 0xFFFFFF)
                #print("offs=%08X next=%08X len=%d" % (offs, oNext, dlen))
                if dlen == 0: # no data
                    self.curves.append(None)
                    continue
                file.seek(offs & 0xFFFFFF)
                data = file.read(dlen)
                if data[0:4] in (b'SEQA', b'SEQB'):
                    # the first 32 entries don't have this header and also
                    # don't have the high bit set.
                    if not offs & 0x80000000: continue
                curv = AnimCurv.fromBytes(data)
                curv.offset = offs & 0xFFFFFF
                self.curves.append(curv)

class AnimCurvXmlWriter:
    """Writes ANIMCURV XML files."""

    def __init__(self, xmlPath:os.PathLike):
        self.xmlPath = xmlPath
        self.root = ET.Element('animcurv')

    def write(self, curves:list[AnimCurv]) -> None:
        """Write the file."""
        for i, curve in enumerate(curves):
            if curve is None: continue
            self.root.append(ET.Comment(" 0x%08X " % ( curve.offset )))
            eCurve = ET.SubElement(self.root, 'curve', {'id':str(i)})
            if curve.signature:
                eCurve.attrib['signature'] = curve.signature
            for action in curve.actions:
                ET.SubElement(eCurve, 'action', {
                    'time':  str(action.time),
                    'cmd':   action.command.name,
                    'param': str(action.param),
                })
            for field, points in curve.points.items():
                for point in points:
                    ET.SubElement(eCurve, 'point', {
                        'field': field.name,
                        'x': str(point.x),
                        'y': str(point.y),
                        'type': str(point.type),
                        'scale': str(point.scale),
                        'unk05': str(point.unk05),
                    })
        ET.ElementTree(self.root).write(self.xmlPath)

class App:
    """The main program."""

    def showUsage(self) -> None:
        print("Usage: %s pack|unpack ANIMCURV.xml ANIMCURV.bin ANIMCURV.tab" % sys.argv[0])

    def pack(self, xmlPath:os.PathLike, binPath:os.PathLike, tabPath:os.PathLike) -> None:
        """Pack animcurv from XML."""
        encoder = RomlistEncoder(self._xmlPath)
        objdefs = encoder.readRomlistXml(inPath)
        with open(outPath, 'wb') as outFile:
            encoder.packRomlist(objdefs, outFile)

    def unpack(self, xmlPath:os.PathLike, binPath:os.PathLike, tabPath:os.PathLike) -> None:
        """Unpack animcurv to XML."""
        decoder = AnimCurvReader(binPath, tabPath)
        decoder.decode()
        AnimCurvXmlWriter(xmlPath).write(decoder.curves)

    def main(self, args:list[str]) -> None:
        try:
            mode    = args.pop(0)
            xmlPath = args.pop(0)
            binPath = args.pop(0)
            tabPath = args.pop(0)
        except IndexError:
            self.showUsage()
            return

        if   mode == 'unpack': self.unpack(xmlPath, binPath, tabPath)
        elif mode == 'pack':   self.pack  (xmlPath, binPath, tabPath)
        else: self.showUsage()

if __name__ == '__main__':
    App().main(list(sys.argv[1:]))
