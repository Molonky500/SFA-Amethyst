#!/usr/bin/env python3
from __future__ import annotations
"""Build/unpack romlist files.

Does not handle compression; use zlb.py for that.
"""

from typing import BinaryIO, TextIO
from pathlib import Path
import os
import sys
import struct
import xml.etree.ElementTree as ET

types = {
    's8':  'b',   'u8':  'B',
    's16': 'h',   'u16': 'H',
    's32': 'i',   'u32': 'I',
    'float': 'f', 'double': 'd',
    'bool': 'b',
    'byte': 'b',
    'char': 'b', # not 'c' because this game doesn't actually use chars here.
    'GameBit': 'h',
    'GameBit16': 'h',
    'GameBit32': 'i',
    'int': 'i',
    'ObjDefEnum': 'h',
    'ObjectId': 'i',
    'pad8': 'b',
    'pad16': 'h',
    'pad32': 'i',
    'pointer': 'I',
    'RomListObjLoadFlags': 'B',
    'short': 'h',
    'undefined': 'b',
    'undefined1': 'b',
    'undefined2': 'h',
    'undefined4': 'i',
    'ubyte': 'B',
    'uint': 'I',
    'ushort': 'H',
    'unk8': 'B',
    'unk16': 'H',
    'unk32': 'I',
    'vec3s': '3h',
    'vec3f': '3f',
}

class Vec3f:
    x: float
    y: float
    z: float

    def __init__(self, x:float=0, y:float=0, z:float=0) -> None:
        self.x = x
        self.y = y
        self.z = z

    @staticmethod
    def fromString(s:str) -> Vec3f:
        vals = s.split()
        return Vec3f(float(vals[0]), float(vals[1]), float(vals[2]))

    def __str__(self) -> str:
        return "%f %f %f" % (self.x, self.y, self.z)


class ObjParam:
    """A parameter in an objdef."""

    type: str
    """The type, which can be a struct format string or
    a key in the `types` dict above.
    """

    _name: str
    """The parameter name."""

    offset: int
    """Offset into the objdef struct of this parameter.

    This includes the common fields, so the first parameter is at 0x18.
    """

    def __init__(self, type:str=None, name:str=None, offset:int=None) -> None:
        self.type   = type
        self._name  = name
        self.offset = offset

    def __repr__(self) -> str:
        if self.offset is None:
            return '<ObjParam "%s", offset unknown>' % self.name
        else: return '<ObjParam "%s", offset 0x%X>' % (self.name, self.offset)

    @property
    def structFmt(self) -> str:
        """The struct format string for this parameter."""
        if self.type in types:
            return '>' + types[self.type]
        else:
            try:
                struct.calcsize('>' + self.type) # does it parse?
                return '>' + self.type # then it's valid
            except:
                raise TypeError("Unrecognized type %r" % self.type)

    @property
    def name(self) -> str:
        """The parameter name."""
        if self._name is None:
            self._name = 'param%02X' % self.offset
        return self._name

    @name.setter
    def name(self, name:str) -> None:
        self._name = name


class DLL:
    """A DLL from the game.

    This class only contains the information needed for romlists.
    """

    id: int
    """The DLL's ID."""

    name: str
    """The DLL's name, if known."""

    objParams: dict[str,ObjParam] = None
    """The objdef parameters for objects using this DLL, indexed by name."""

    def __init__(self, id:int=None, name:str=None, objParams:dict={}) -> None:
        self.id = id
        self.name = name
        self.objParams = objParams.copy()

    def __str__(self) -> str:
        return "<DLL #0x%04X: %s>" % (self.id, str(self.name))


class ObjClass:
    """A type of object in the game."""

    id: int
    """The object ID."""

    defNo: int
    """The ID that objdef uses to refer to this object type."""

    dllId: int
    """The DLL ID that this object uses."""

    name: str
    """The object's name, extracted from the game files."""

    def __init__(self, id:int=None, defNo:int=None, dllId:int=None,
    name:str=None) -> None:
        self.id    = id
        self.defNo = defNo
        self.dllId = dllId
        self.name  = name


class ObjDef:
    """The ObjDef struct in the game. Defines one object instance."""

    objType: int
    """The object type ID."""

    allocatedSize: int
    """The total number of 32-bit words this objdef contains."""

    mapActs1: int
    """Part 1 of the flags that tell which map acts to NOT load this object in."""

    mapActs2: int
    """Part 2 of the flags that tell which map acts to NOT load this object in."""

    loadFlags: int
    """Flags that control when to load the object."""

    bound: int
    """Defines the area in which to load the object."""

    cullDist: int
    """Defines the area in which to render the object."""

    pos: Vec3f
    """The object's coordinates, relative to the map's origin."""

    id: int
    """The object's "unique" ID.

    This is supposed to be unique to every object in the game.
    However, in some unused maps, there are duplicate IDs.
    Also, dynamically created objects have ID -1.
    """

    params: dict[str,any]
    """The parameters specific to this object type."""

    _struct = ('>'
         'H' # ObjDefEnum objType
         'B' # u8 allocatedSize (number of words)
         'B' # u8 mapActs1
         'B' # RomListObjLoadFlags loadFlags
         'B' # u8 mapActs2
         'B' # u8 bound
         'B' # u8 cullDist
        '3f' # vec3f pos
         'i' # int id
    )
    _structLen = struct.calcsize(_struct)

    # weird order to match game struct
    def __init__(self, objType:int=0, allocatedSize:int=0, mapActs1=0,
    loadFlags=0, mapActs2=0, bound=0, cullDist=0, pos=Vec3f(),
    id=0, params={}) -> None:
        self.objType       = objType
        self.allocatedSize = allocatedSize
        self.mapActs1      = mapActs1
        self.loadFlags     = loadFlags
        self.mapActs2      = mapActs2
        self.bound         = bound
        self.cullDist      = cullDist
        self.pos           = pos
        self.id            = id
        self.params        = params.copy()

    @staticmethod
    def fromBytes(data:bytes) -> ObjDef:
        """Decode ObjDef from bytes.

        This does not decode the parameters.
        """
        data = struct.unpack(ObjDef._struct, data)
        return ObjDef(
            objType       = data[0],
            allocatedSize = data[1],
            mapActs1      = data[2],
            loadFlags     = data[3],
            mapActs2      = data[4],
            bound         = data[5],
            cullDist      = data[6],
            pos           = Vec3f(data[7], data[8], data[9]),
            id            = data[10],
        )

    def toBytes(self) -> bytes:
        """Encode ObjDef to bytes.

        This does not include the parameters."""
        return struct.pack(self._struct,
            self.objType,
            self.allocatedSize,
            self.mapActs1,
            self.loadFlags,
            self.mapActs2,
            self.bound,
            self.cullDist,
            self.pos.x,
            self.pos.y,
            self.pos.z,
            self.id,
        )

    def toXmlAttrs(self) -> dict[str,str]:
        """Build XML attributes table."""
        return {
            'objType':       str(self.objType),
            'allocatedSize': str(self.allocatedSize),
            'mapActs1':      str(self.mapActs1),
            'loadFlags':     str(self.loadFlags),
            'mapActs2':      str(self.mapActs2),
            'bound':         str(self.bound),
            'cullDist':      str(self.cullDist),
            'pos':           str(self.pos),
            'id':            str(self.id),
        }

    def __str__(self) -> str:
        attrs = self.toXmlAttrs()
        result = []
        for k, v in attrs.items():
            result.append("%s=%s" % (k, str(v)))
        return "ObjDef(%s)" % (', '.join(result))


class XmlHandler:
    """Base for classes that operate on the xml files."""

    _xmlPath: os.PathLike
    """Path to the directory containing objects.xml and dlls.xml."""

    def __init__(self, xmlPath:os.PathLike) -> None:
        self._xmlPath = Path(xmlPath)

class RomlistHandler(XmlHandler):
    """Base for classes that operate on romlists."""

    objTypes: dict[int, ObjClass]
    """The object types, indexed by defNo."""

    dlls: dict[int, DLL]
    """The DLLs, indexed by ID."""

    def __init__(self, xmlPath:os.PathLike) -> None:
        super().__init__(xmlPath)
        self._readObjectsXml()
        self._readDllsXml()

    def getObjDll(self, objType:int) -> DLL:
        """Get the DLL for the given objdef type."""
        try: obj = self.objTypes[objType]
        except KeyError:
            print("Unknown objdef", objType)
            return None
        try: dll = self.dlls[obj.dllId]
        except KeyError:
            print("Unknown DLL ID", obj.dllId)
            return None
        return dll

    def _readObjectsXml(self) -> None:
        """Read objdef types from objects.xml.

        Populate self.objTypes with the read data.
        """
        root = ET.parse(self._xmlPath / 'objects.xml').getroot()
        self.objTypes = {}
        for eObject in root.findall('./object'):
            try:
                obj = ObjClass(
                    id    = int(eObject.get('id'), 0),
                    name  = str(eObject.get('name', '')),
                    defNo = int(eObject.get('def'), 0),
                    dllId = int(eObject.get('dll', '0'), 0),
                )
                self.objTypes[obj.defNo] = obj
            except:
                print("Error parsing object element:", eObject.tag)
                for k, v in eObject.attrib.items(): print("", k, v)
                raise

    def _readObjParam(self, eParam:ET.Element) -> ObjParam:
        """Read an object parameter from a DLL."""
        # skip ones with no offset (included sometimes as notes)
        if eParam.get('offset', None) is None: return None
        try:
            return ObjParam(
                offset = int(eParam.get('offset'), 0),
                type = eParam.get('type'),
                name = eParam.get('name'),
            )
        except:
            print("Error parsing DLL element:", eParam.tag)
            for k, v in eParam.attrib.items(): print("", k, v)
            raise

    def _readDllsXml(self) -> None:
        """Read DLL parameters from dlls.xml.

        Populate self.dlls with the read data.
        """
        root = ET.parse(self._xmlPath / 'dlls.xml').getroot()
        self.dlls = {}
        for eDll in root.findall('./dll'):
            id = int(eDll.get('id'), 0)
            dll = DLL(id=id, name=eDll.get('name', ''))
            self.dlls[dll.id] = dll
            eObjParams = eDll.find('./objparams')
            if eObjParams is not None:
                for eParam in eObjParams.findall('./param'):
                    param = self._readObjParam(eParam)
                    if param: dll.objParams[param.name] = param


class RomlistDecoder(RomlistHandler):
    """Decodes binary romlist files into XML."""

    def decodeObjdefParams(self, objdef:ObjDef, inFile:BinaryIO) -> None:
        """Fill in objdef.params from inFile."""
        # get the parameter definitions from the DLL
        dll = self.getObjDll(objdef.objType)
        objParams = dll.objParams if dll else {}
        paramLen = (objdef.allocatedSize * 4) - ObjDef._structLen
        data = inFile.read(paramLen)
        seenOffsets = set()
        for name, param in objParams.items():
            # fill in the offsets this parameter covers
            tSize = struct.calcsize(param.structFmt)
            for i in range(tSize):
                seenOffsets.add(i+param.offset)

            # decode the parameter
            try:
                lol = struct.unpack_from(param.structFmt, data,
                    param.offset - ObjDef._structLen)
                if len(lol) == 1: lol = lol[0] # grumble
            except:
                print("Error unpacking param", repr(param))
                raise
            objdef.params[name] = {
                'value':  lol,
                'offset': param.offset,
                'type':   param.type,
            }

        # fill in unknowns
        for i in range(ObjDef._structLen, paramLen+ObjDef._structLen):
            if i not in seenOffsets:
                val = struct.unpack_from(
                    '>B', data, i - ObjDef._structLen)[0] # grumble
                objdef.params['param%02X' % i] = {
                    'value':  val,
                    'offset': i,
                    'type':   'unk8',
                }

    def unpackRomlist(self, inFile:BinaryIO) -> list[ObjDef]:
        """Unpack binary romlist file."""
        result = []
        while True:
            offs = inFile.tell() # debug
            data = inFile.read(ObjDef._structLen)
            if (not data) or len(data) < ObjDef._structLen: break
            objdef = ObjDef.fromBytes(data)
            try: self.decodeObjdefParams(objdef, inFile)
            except:
                print("Error decoding objdef at 0x%X: %s" % (
                    offs, objdef))
                raise
            result.append(objdef)
        return result

    def writeRomListXml(self, objs:list[ObjDef], path:os.PathLike) -> None:
        """Write decoded XML romlist to given path."""
        root = ET.Element('romlist')
        for objdef in objs:
            # add a helpful comment
            root.append(ET.Comment(" %s 0x%08X " % (
                str(self.objTypes[objdef.objType].name),
                objdef.id,
            )))

            # create the objdef element
            eObjdef = ET.SubElement(root, 'objdef', objdef.toXmlAttrs())
            for name, val in objdef.params.items():
                if val['type'] == 'vec3s': # HACK
                    value = '%d %d %d' % val['value']
                else: value = str(val['value'])
                ET.SubElement(eObjdef, 'param', {
                    'name':  str(name),
                    'type':  str(val['type']),
                    'offset': '0x%X' % val['offset'],
                    'value': value,
                })
        ET.ElementTree(root).write(path)

class RomlistEncoder(RomlistHandler):
    """Encodes binary romlist files from XML."""

    def _readObjdefElem(self, eObjdef:ET.Element) -> ObjDef:
        """Read XML objdef element."""
        def _intAttr(val):
            return int(val, 0)
        attrs = {
            # name is only for readability; it's not used here
            'objType':       _intAttr,
            'allocatedSize': _intAttr,
            'mapActs1':      _intAttr,
            'loadFlags':     _intAttr,
            'mapActs2':      _intAttr,
            'bound':         _intAttr,
            'cullDist':      _intAttr,
            'pos':           lambda v: Vec3f.fromString(v),
            'id':            _intAttr,
        }
        result = {'params':{}}
        for name, func in attrs.items():
            result[name] = func(eObjdef.get(name))
        for eParam in eObjdef.findall('./param'):
            typ = eParam.get('type')
            val = eParam.get('value')
            if typ == 'float': val = float(val)
            elif typ == 'vec3s':
                val = val.split()
                val = (int(val[0]), int(val[1]), int(val[2]))
                typ = '3h'
            else: val = int(val, 0)
            result['params'][eParam.get('name')] = {
                'value':  val,
                'type':   typ,
                'offset': int(eParam.get('offset'), 0),
            }
        return ObjDef(**result)

    def readRomlistXml(self, path:os.PathLike) -> list[ObjDef]:
        """Read XML romlist."""
        root = ET.parse(path).getroot()
        result = []
        for eObjdef in root.findall('./objdef'):
            objdef = self._readObjdefElem(eObjdef)
            result.append(objdef)
        return result

    def packRomlist(self, objdefs:list[ObjDef], outFile:BinaryIO) -> None:
        """Write binary romlist."""
        for objdef in objdefs:
            outFile.write(objdef.toBytes())
            # we need this if we write the params without type and offset.
            # but doing that means we need to have the params for every DLL.
            #dll = self.getObjDll(objdef.objType)
            #objParams = dll.objParams if dll else {}

            # ensure the parameters are accounted for
            #for name in objdef.params.keys():
            #    if name not in objParams:
            #        print("ERROR: Missing parameter '%s' for DLL %s" % (
            #            name, dll))

            # write the parameters
            seenOffsets = set()
            paramData = {}
            for name, param in objdef.params.items():
                try:
                    typ = '>' + types.get(param['type'], param['type'])
                    tSize = struct.calcsize(typ)
                    for i in range(tSize):
                        seenOffsets.add(i+param['offset'])
                    val = param['value']
                    #if param['offset'] in paramData:
                    #    print("Warning: overlapping params for %s at 0x%X" % (
                    #        str(objdef), param['offset']))
                    if type(val) in (list, tuple): # lol
                        paramData[param['offset']] = struct.pack(typ, *val)
                    else: paramData[param['offset']] = struct.pack(typ, val)
                except:
                    print("Error packing param", param)
                    raise
            for i in range(ObjDef._structLen, objdef.allocatedSize * 4):
                if i not in seenOffsets:
                    print("ERROR: No value for offset 0x%X of %s" % (
                        i, str(objdef)))
                    raise ValueError("Missing value")
            # overlapping parameters are valid (some objdefs have unions)
            # but if they're different size, we need to ensure we don't
            # write too many bytes.
            # eg if we have union { struct {u16 x, u16 y}; u32 z}
            # then we'd put x at 0x00, y at 0x02, z at 0x00.
            # if we see z last, it will replace x in paramData, but
            # won't replace y, so we'd end up writing both y and z
            # sequentially, when one should replace the other.
            # XXX properly handle unions somehow.
            base = outFile.tell()
            for i in range(ObjDef._structLen, objdef.allocatedSize * 4):
                if i not in paramData: continue
                outFile.seek(base+i-ObjDef._structLen)
                outFile.write(paramData[i])
        # write EOF marker
        #outFile.write(b'\0\0\0\xFF')

        # pad file to 32 bytes
        #offs = outFile.tell()
        #nPad = offs & 31
        #if nPad:
        #    nPad = 32 - nPad
        #    outFile.write(b'\0' * nPad)


class MapsBinRomlistUpdater(XmlHandler):
    """Updates romlist sizes in MAPS.bin."""

    romlistMapIds: dict[str,int]
    """Dict of romlist name => map ID."""

    root: Path
    """Path to game's root directory."""

    def __init__(self, xmlPath:os.PathLike, root:os.PathLike) -> None:
        super().__init__(xmlPath)
        self.root = Path(root)
        self._readMapsXml()

    def _readMapsXml(self) -> None:
        """Read map data from maps.xml.

        Populate self.romlistMapIds with the read data.
        """
        root = ET.parse(self._xmlPath / 'maps.xml').getroot()
        self.romlistMapIds = {}
        for eMap in root.findall('./map'):
            try:
                romlist = eMap.get('romlist', None)
                id = eMap.get('id', None) # not dirid
                if id is None or romlist is None: continue
                self.romlistMapIds[romlist] = int(id, 0)
            except:
                print("Error parsing map element:", eMap.tag)
                for k, v in eMap.attrib.items(): print("", k, v)
                raise

    def getMapIdForRomlist(self, romlist:str) -> int:
        """Get the map ID for the romlist name."""
        return self.romlistMapIds.get(romlist, None)

    def calcTabEntryOffset(self, mapId:int) -> int:
        """Calculate the location in MAPS.tab of this map's offsets."""
        return mapId * 7 * 4 # 7 u32 values
        #infoOffset, blockTable, rects1, rects2, rects3, rects4, listSize

    def setMapRomlistSize(self, mapId:int, size:int) -> None:
        """Write the romlist size in MAPS.bin."""
        assert size < 65536, "Romlist too large (max 65535 bytes)"
        with open(self.root / 'MAPS.tab', 'rb') as mapsTab:
            mapsTab.seek(self.calcTabEntryOffset(mapId))
            offsInfo = struct.unpack('>I', mapsTab.read(4))[0] # grumble
            mapsTab.seek(self.calcTabEntryOffset(mapId) + (6*4))
            offsFacefeed = struct.unpack('>I', mapsTab.read(4))[0] # grumble

        #print("MAPS.bin offset: 0x%X" % offsInfo)
        with open(self.root / 'MAPS.bin', 'r+b') as mapsBin:
            mapsBin.seek(offsInfo+8)
            mapsBin.write(struct.pack('>H', size))
            mapsBin.seek(offsFacefeed+4)
            mapsBin.write(struct.pack('>I', size))


class App:
    """The main program."""

    _xmlPath: os.PathLike
    """Path to the directory containing objects.xml and dlls.xml."""

    def showUsage(self) -> None:
        print("Usage: %s MODE XMLPATH INPATH OUTPATH" % sys.argv[0])
        print(
            "MODE is 'pack', 'unpack', or 'updateSize'.\n"
            "XMLPATH points to directory containing dlls.xml and objects.xml\n"
            "For updateSize, INPATH is the romlist.bin file, and OUTPATH is\n"
            "the game's root directory.")

    def pack(self, inPath:os.PathLike, outPath:os.PathLike) -> None:
        """Pack romlist from XML."""
        encoder = RomlistEncoder(self._xmlPath)
        objdefs = encoder.readRomlistXml(inPath)
        with open(outPath, 'wb') as outFile:
            encoder.packRomlist(objdefs, outFile)

    def unpack(self, inPath:os.PathLike, outPath:os.PathLike) -> None:
        """Unpack romlist to XML."""
        decoder = RomlistDecoder(self._xmlPath)
        with open(inPath, 'rb') as inFile:
            objdefs = decoder.unpackRomlist(inFile)
        decoder.writeRomListXml(objdefs, outPath)

    def updateSize(self, romlist:os.PathLike, root:os.PathLike) -> None:
        """Set a map's romlist size to match the file.

        :param romlist: The path to the uncompressed romlist.bin file.
        :param root: The path to the game's root directory.

        Writes the romlist file's size into the MAPS.bin entry corresponding
        to the romlist file's name.
        """
        updater = MapsBinRomlistUpdater(self._xmlPath, root)
        romlist = Path(romlist)
        mapName = romlist.name.split('.')[0]
        mapId   = updater.getMapIdForRomlist(mapName)
        if mapId is None:
            raise ValueError("No such map: '%s'" % str(mapName))
        size = os.stat(romlist).st_size
        #print("Set size of map ID 0x%X to 0x%X (%s)" % (mapId, size, mapName))
        updater.setMapRomlistSize(mapId, size)

    def main(self, args:list[str]) -> None:
        try:
            mode = args.pop(0)
            self._xmlPath = args.pop(0)
            inPath  = args.pop(0)
            outPath = args.pop(0)
        except IndexError:
            self.showUsage()
            return

        if   mode == 'unpack': self.unpack(inPath, outPath)
        elif mode == 'pack':   self.pack  (inPath, outPath)
        elif mode == 'updateSize': self.updateSize(inPath, outPath)
        else: self.showUsage()

if __name__ == '__main__':
    App().main(list(sys.argv[1:]))
