"""biplist -- a library for reading and writing binary property list files.

Binary Property List (plist) files provide a faster and smaller serialization
format for property lists on OS X. This is a library for generating binary
plists which can be read by OS X, iOS, or other clients.

The API models the plistlib API, and will call through to plistlib when
XML serialization or deserialization is required.

To generate plists with UID values, wrap the values with the Uid object. The
value must be an int.

To generate plists with NSData/CFData values, wrap the values with the
Data object. The value must be a string.

Date values can only be datetime.datetime objects.

The exceptions InvalidPlistException and NotBinaryPlistException may be 
thrown to indicate that the data cannot be serialized or deserialized as
a binary plist.

Plist generation example:
    
    from biplist import *
    from datetime import datetime
    plist = {'aKey':'aValue',
             '0':1.322,
             'now':datetime.now(),
             'list':[1,2,3],
             'tuple':('a','b','c')
             }
    try:
        writePlist(plist, "example.plist")
    except (InvalidPlistException, NotBinaryPlistException), e:
        print "Something bad happened:", e

Plist parsing example:

    from biplist import *
    try:
        plist = readPlist("example.plist")
        print plist
    except (InvalidPlistException, NotBinaryPlistException), e:
        print "Not a plist:", e
"""

from collections import namedtuple
import datetime
import io
import math
import plistlib
from struct import pack, unpack, unpack_from
from struct import error as struct_error
import sys
import time

try:
    unicode
    unicodeEmpty = r''
except NameError:
    unicode = str
    unicodeEmpty = ''
try:
    long
except NameError:
    long = int
try:
    {}.iteritems
    iteritems = lambda x: x.iteritems()
except AttributeError:
    iteritems = lambda x: x.items()

__all__ = [
    'Uid', 'Data', 'readPlist', 'writePlist', 'readPlistFromString',
    'writePlistToString', 'InvalidPlistException', 'NotBinaryPlistException'
]

# Apple uses Jan 1, 2001 as a base for all plist date/times.
apple_reference_date = datetime.datetime.utcfromtimestamp(978307200)

class Uid(object):
    """Wrapper around integers for representing UID values. This
       is used in keyed archiving."""
    integer = 0
    def __init__(self, integer):
        self.integer = integer
    
    def __repr__(self):
        return "Uid(%d)" % self.integer
    
    def __eq__(self, other):
        if isinstance(self, Uid) and isinstance(other, Uid):
            return self.integer == other.integer
        return False
    
    def __cmp__(self, other):
        return self.integer - other.integer
    
    def __lt__(self, other):
        return self.integer < other.integer
    
    def __hash__(self):
        return self.integer
    
    def __int__(self):
        return int(self.integer)

class Data(bytes):
    """Wrapper around bytes to distinguish Data values."""

class InvalidPlistException(Exception):
    """Raised when the plist is incorrectly formatted."""

class NotBinaryPlistException(Exception):
    """Raised when a binary plist was expected but not encountered."""

def readPlist(pathOrFile):
    """Raises NotBinaryPlistException, InvalidPlistException"""
    didOpen = False
    result = None
    if isinstance(pathOrFile, (bytes, unicode)):
        pathOrFile = open(pathOrFile, 'rb')
        didOpen = True
    try:
        reader = PlistReader(pathOrFile)
        result = reader.parse()
    except NotBinaryPlistException as e:
        try:
            pathOrFile.seek(0)
            result = None
            if hasattr(plistlib, 'loads'):
                contents = None
                if isinstance(pathOrFile, (bytes, unicode)):
                    with open(pathOrFile, 'rb') as f:
                        contents = f.read()
                else:
                    contents = pathOrFile.read()
                result = plistlib.loads(contents)
            else:
                result = plistlib.readPlist(pathOrFile)
            result = wrapDataObject(result, for_binary=True)
        except Exception as e:
            raise InvalidPlistException(e)
    finally:
        if didOpen:
            pathOrFile.close()
    return result

def wrapDataObject(o, for_binary=False):
    if isinstance(o, Data) and not for_binary:
        v = sys.version_info
        if not (v[0] >= 3 and v[1] >= 4):
            o = plistlib.Data(o)
    elif isinstance(o, (bytes, plistlib.Data)) and for_binary:
        if hasattr(o, 'data'):
            o = Data(o.data)
    elif isinstance(o, tuple):
        o = wrapDataObject(list(o), for_binary)
        o = tuple(o)
    elif isinstance(o, list):
        for i in range(len(o)):
            o[i] = wrapDataObject(o[i], for_binary)
    elif isinstance(o, dict):
        for k in o:
            o[k] = wrapDataObject(o[k], for_binary)
    return o

def writePlist(rootObject, pathOrFile, binary=True):
    if not binary:
        rootObject = wrapDataObject(rootObject, binary)
        if hasattr(plistlib, "dump"):
            if isinstance(pathOrFile, (bytes, unicode)):
                with open(pathOrFile, 'wb') as f:
                    return plistlib.dump(rootObject, f)
            else:
                return plistlib.dump(rootObject, pathOrFile)
        else:
            return plistlib.writePlist(rootObject, pathOrFile)
    else:
        didOpen = False
        if isinstance(pathOrFile, (bytes, unicode)):
            pathOrFile = open(pathOrFile, 'wb')
            didOpen = True
        writer = PlistWriter(pathOrFile)
        result = writer.writeRoot(rootObject)
        if didOpen:
            pathOrFile.close()
        return result

def readPlistFromString(data):
    return readPlist(io.BytesIO(data))

def writePlistToString(rootObject, binary=True):
    if not binary:
        rootObject = wrapDataObject(rootObject, binary)
        if hasattr(plistlib, "dumps"):
            return plistlib.dumps(rootObject)
        elif hasattr(plistlib, "writePlistToBytes"):
            return plistlib.writePlistToBytes(rootObject)
        else:
            return plistlib.writePlistToString(rootObject)
    else:
        ioObject = io.BytesIO()
        writer = PlistWriter(ioObject)
        writer.writeRoot(rootObject)
        return ioObject.getvalue()

def is_stream_binary_plist(stream):
    stream.seek(0)
    header = stream.read(7)
    if header == b'bplist0':
        return True
    else:
        return False

PlistTrailer = namedtuple('PlistTrailer', 'offsetSize, objectRefSize, offsetCount, topLevelObjectNumber, offsetTableOffset')
PlistByteCounts = namedtuple('PlistByteCounts', 'nullBytes, boolBytes, intBytes, realBytes, dateBytes, dataBytes, stringBytes, uidBytes, arrayBytes, setBytes, dictBytes')

class PlistReader(object):
    file = None
    contents = ''
    offsets = None
    trailer = None
    currentOffset = 0
    
    def __init__(self, fileOrStream):
        """Raises NotBinaryPlistException."""
        self.reset()
        self.file = fileOrStream
    
    def parse(self):
        return self.readRoot()
    
    def reset(self):
        self.trailer = None
        self.contents = ''
        self.offsets = []
        self.currentOffset = 0
    
    def readRoot(self):
        result = None
        self.reset()
        # Get the header, make sure it's a valid file.
        if not is_stream_binary_plist(self.file):
            raise NotBinaryPlistException()
        self.file.seek(0)
        self.contents = self.file.read()
        if len(self.contents) < 32:
            raise InvalidPlistException("File is too short.")
        trailerContents = self.contents[-32:]
        try:
            self.trailer = PlistTrailer._make(unpack("!xxxxxxBBQQQ", trailerContents))
            offset_size = self.trailer.offsetSize * self.trailer.offsetCount
            offset = self.trailer.offsetTableOffset
            offset_contents = self.contents[offset:offset+offset_size]
            offset_i = 0
            while offset_i < self.trailer.offsetCount:
                begin = self.trailer.offsetSize*offset_i
                tmp_contents = offset_contents[begin:begin+self.trailer.offsetSize]
                tmp_sized = self.getSizedInteger(tmp_contents, self.trailer.offsetSize)
                self.offsets.append(tmp_sized)
                offset_i += 1
            self.setCurrentOffsetToObjectNumber(self.trailer.topLevelObjectNumber)
            result = self.readObject()
        except TypeError as e:
            raise InvalidPlistException(e)
        return result
    
    def setCurrentOffsetToObjectNumber(self, objectNumber):
        self.currentOffset = self.offsets[objectNumber]
    
    def readObject(self):
        result = None
        tmp_byte = self.contents[self.currentOffset:self.currentOffset+1]
        marker_byte = unpack("!B", tmp_byte)[0]
        format = (marker_byte >> 4) & 0x0f
        extra = marker_byte & 0x0f
        self.currentOffset += 1
        
        def proc_extra(extra):
            if extra == 0b1111:
                #self.currentOffset += 1
                extra = self.readObject()
            return extra
        
        # bool, null, or fill byte
        if format == 0b0000:
            if extra == 0b0000:
                result = None
            elif extra == 0b1000:
                result = False
            elif extra == 0b1001:
                result = True
            elif extra == 0b1111:
                pass # fill byte
            else:
                raise InvalidPlistException("Invalid object found at offset: %d" % (self.currentOffset - 1))
        # int
        elif format == 0b0001:
            extra = proc_extra(extra)
            result = self.readInteger(pow(2, extra))
        # real
        elif format == 0b0010:
            extra = proc_extra(extra)
            result = self.readReal(extra)
        # date
        elif format == 0b0011 and extra == 0b0011:
            result = self.readDate()
        # data
        elif format == 0b0100:
            extra = proc_extra(extra)
            result = self.readData(extra)
        # ascii string
        elif format == 0b0101:
            extra = proc_extra(extra)
            result = self.readAsciiString(extra)
        # Unicode string
        elif format == 0b0110:
            extra = proc_extra(extra)
            result = self.readUnicode(extra)
        # uid
        elif format == 0b1000:
            result = self.readUid(extra)
        # array
        elif format == 0b1010:
            extra = proc_extra(extra)
            result = self.readArray(extra)
        # set
        elif format == 0b1100:
            extra = proc_extra(extra)
            result = set(self.readArray(extra))
        # dict
        elif format == 0b1101:
            extra = proc_extra(extra)
            result = self.readDict(extra)
        else:    
            raise InvalidPlistException("Invalid object found: {format: %s, extra: %s}" % (bin(format), bin(extra)))
        return result
    
    def readInteger(self, byteSize):
        result = 0
        original_offset = self.currentOffset
        data = self.contents[self.currentOffset:self.currentOffset + byteSize]
        result = self.getSizedInteger(data, byteSize, as_number=True)
        self.currentOffset = original_offset + byteSize
        return result
    
    def readReal(self, length):
        result = 0.0
        to_read = pow(2, length)
        data = self.contents[self.currentOffset:self.currentOffset+to_read]
        if length == 2: # 4 bytes
            result = unpack('>f', data)[0]
        elif length == 3: # 8 bytes
            result = unpack('>d', data)[0]
        else:
            raise InvalidPlistException("Unknown real of length %d bytes" % to_read)
        return result
    
    def readRefs(self, count):    
        refs = []
        i = 0
        while i < count:
            fragment = self.contents[self.currentOffset:self.currentOffset+self.trailer.objectRefSize]
            ref = self.getSizedInteger(fragment, len(fragment))
            refs.append(ref)
            self.currentOffset += self.trailer.objectRefSize
            i += 1
        return refs
    
    def readArray(self, count):
        result = []
        values = self.readRefs(count)
        i = 0
        while i < len(values):
            self.setCurrentOffsetToObjectNumber(values[i])
            value = self.readObject()
            result.append(value)
            i += 1
        return result
    
    def readDict(self, count):
        result = {}
        keys = self.readRefs(count)
        values = self.readRefs(count)
        i = 0
        while i < len(keys):
            self.setCurrentOffsetToObjectNumber(keys[i])
            key = self.readObject()
            self.setCurrentOffsetToObjectNumber(values[i])
            value = self.readObject()
            result[key] = value
            i += 1
        return result
    
    def readAsciiString(self, length):
        result = unpack("!%ds" % length, self.contents[self.currentOffset:self.currentOffset+length])[0]
        self.currentOffset += length
        return str(result.decode('ascii'))
    
    def readUnicode(self, length):
        actual_length = length*2
        data = self.contents[self.currentOffset:self.currentOffset+actual_length]
        # unpack not needed?!! data = unpack(">%ds" % (actual_length), data)[0]
        self.currentOffset += actual_length
        return data.decode('utf_16_be')
    
    def readDate(self):
        result = unpack(">d", self.contents[self.currentOffset:self.currentOffset+8])[0]
        # Use timedelta to workaround time_t size limitation on 32-bit python.
        result = datetime.timedelta(seconds=result) + apple_reference_date
        self.currentOffset += 8
        return result
    
    def readData(self, length):
        result = self.contents[self.currentOffset:self.currentOffset+length]
        self.currentOffset += length
        return Data(result)
    
    def readUid(self, length):
        return Uid(self.readInteger(length+1))
    
    def getSizedInteger(self, data, byteSize, as_number=False):
        """Numbers of 8 bytes are signed integers when they refer to numbers, but unsigned otherwise."""
        result = 0
        # 1, 2, and 4 byte integers are unsigned
        if byteSize == 1:
            result = unpack('>B', data)[0]
        elif byteSize == 2:
            result = unpack('>H', data)[0]
        elif byteSize == 4:
            result = unpack('>L', data)[0]
        elif byteSize == 8:
            if as_number:
                result = unpack('>q', data)[0]
            else:
                result = unpack('>Q', data)[0]
        elif byteSize <= 16:
            # Handle odd-sized or integers larger than 8 bytes
            # Don't naively go over 16 bytes, in order to prevent infinite loops.
            result = 0
            if hasattr(int, 'from_bytes'):
                result = int.from_bytes(data, 'big')
            else:
                for byte in data:
                    if not isinstance(byte, int): # Python3.0-3.1.x return ints, 2.x return str
                        byte = unpack_from('>B', byte)[0]
                    result = (result << 8) | byte
        else:
            raise InvalidPlistException("Encountered integer longer than 16 bytes.")
        return result

class HashableWrapper(object):
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return "<HashableWrapper: %s>" % [self.value]

class BoolWrapper(object):
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return "<BoolWrapper: %s>" % self.value

class FloatWrapper(object):
    _instances = {}
    def __new__(klass, value):
        # Ensure FloatWrapper(x) for a given float x is always the same object
        wrapper = klass._instances.get(value)
        if wrapper is None:
            wrapper = object.__new__(klass)
            wrapper.value = value
            klass._instances[value] = wrapper
        return wrapper
    def __repr__(self):
        return "<FloatWrapper: %s>" % self.value

class StringWrapper(object):
    __instances = {}
    
    encodedValue = None
    encoding = None
    
    def __new__(cls, value):
        '''Ensure we only have a only one instance for any string,
         and that we encode ascii as 1-byte-per character when possible'''
        
        encodedValue = None
        
        for encoding in ('ascii', 'utf_16_be'):
            try:
               encodedValue = value.encode(encoding)
            except: pass
            if encodedValue is not None:
                if encodedValue not in cls.__instances:
                    cls.__instances[encodedValue] = super(StringWrapper, cls).__new__(cls)
                    cls.__instances[encodedValue].encodedValue = encodedValue
                    cls.__instances[encodedValue].encoding = encoding
                return cls.__instances[encodedValue]
        
        raise ValueError('Unable to get ascii or utf_16_be encoding for %s' % repr(value))
    
    def __len__(self):
        '''Return roughly the number of characters in this string (half the byte length)'''
        if self.encoding == 'ascii':
            return len(self.encodedValue)
        else:
            return len(self.encodedValue)//2
    
    @property
    def encodingMarker(self):
        if self.encoding == 'ascii':
            return 0b0101
        else:
            return 0b0110
    
    def __repr__(self):
        return '<StringWrapper (%s): %s>' % (self.encoding, self.encodedValue)

class PlistWriter(object):
    header = b'bplist00bybiplist1.0'
    file = None
    byteCounts = None
    trailer = None
    computedUniques = None
    writtenReferences = None
    referencePositions = None
    wrappedTrue = None
    wrappedFalse = None
    
    def __init__(self, file):
        self.reset()
        self.file = file
        self.wrappedTrue = BoolWrapper(True)
        self.wrappedFalse = BoolWrapper(False)

    def reset(self):
        self.byteCounts = PlistByteCounts(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        self.trailer = PlistTrailer(0, 0, 0, 0, 0)
        
        # A set of all the uniques which have been computed.
        self.computedUniques = set()
        # A list of all the uniques which have been written.
        self.writtenReferences = {}
        # A dict of the positions of the written uniques.
        self.referencePositions = {}
        
    def positionOfObjectReference(self, obj):
        """If the given object has been written already, return its
           position in the offset table. Otherwise, return None."""
        return self.writtenReferences.get(obj)
        
    def writeRoot(self, root):
        """
        Strategy is:
        - write header
        - wrap root object so everything is hashable
        - compute size of objects which will be written
          - need to do this in order to know how large the object refs
            will be in the list/dict/set reference lists
        - write objects
          - keep objects in writtenReferences
          - keep positions of object references in referencePositions
          - write object references with the length computed previously
        - computer object reference length
        - write object reference positions
        - write trailer
        """
        output = self.header
        wrapped_root = self.wrapRoot(root)
        self.computeOffsets(wrapped_root, asReference=True, isRoot=True)
        self.trailer = self.trailer._replace(**{'objectRefSize':self.intSize(len(self.computedUniques))})
        self.writeObjectReference(wrapped_root, output)
        output = self.writeObject(wrapped_root, output, setReferencePosition=True)
        
        # output size at this point is an upper bound on how big the
        # object reference offsets need to be.
        self.trailer = self.trailer._replace(**{
            'offsetSize':self.intSize(len(output)),
            'offsetCount':len(self.computedUniques),
            'offsetTableOffset':len(output),
            'topLevelObjectNumber':0
            })
        
        output = self.writeOffsetTable(output)
        output += pack('!xxxxxxBBQQQ', *self.trailer)
        self.file.write(output)

    def wrapRoot(self, root):
        if isinstance(root, bool):
            if root is True:
                return self.wrappedTrue
            else:
                return self.wrappedFalse
        elif isinstance(root, float):
            return FloatWrapper(root)
        elif isinstance(root, set):
            n = set()
            for value in root:
                n.add(self.wrapRoot(value))
            return HashableWrapper(n)
        elif isinstance(root, dict):
            n = {}
            for key, value in iteritems(root):
                n[self.wrapRoot(key)] = self.wrapRoot(value)
            return HashableWrapper(n)
        elif isinstance(root, list):
            n = []
            for value in root:
                n.append(self.wrapRoot(value))
            return HashableWrapper(n)
        elif isinstance(root, tuple):
            n = tuple([self.wrapRoot(value) for value in root])
            return HashableWrapper(n)
        elif isinstance(root, (str, unicode)) and not isinstance(root, Data):
            return StringWrapper(root)
        elif isinstance(root, bytes):
            return Data(root)
        else:
            return root

    def incrementByteCount(self, field, incr=1):
        self.byteCounts = self.byteCounts._replace(**{field:self.byteCounts.__getattribute__(field) + incr})

    def computeOffsets(self, obj, asReference=False, isRoot=False):
        def check_key(key):
            if key is None:
                raise InvalidPlistException('Dictionary keys cannot be null in plists.')
            elif isinstance(key, Data):
                raise InvalidPlistException('Data cannot be dictionary keys in plists.')
            elif not isinstance(key, StringWrapper):
                raise InvalidPlistException('Keys must be strings.')
        
        def proc_size(size):
            if size > 0b1110:
                size += self.intSize(size)
            return size
        # If this should be a reference, then we keep a record of it in the
        # uniques table.
        if asReference:
            if obj in self.computedUniques:
                return
            else:
                self.computedUniques.add(obj)
        
        if obj is None:
            self.incrementByteCount('nullBytes')
        elif isinstance(obj, BoolWrapper):
            self.incrementByteCount('boolBytes')
        elif isinstance(obj, Uid):
            size = self.intSize(obj.integer)
            self.incrementByteCount('uidBytes', incr=1+size)
        elif isinstance(obj, (int, long)):
            size = self.intSize(obj)
            self.incrementByteCount('intBytes', incr=1+size)
        elif isinstance(obj, FloatWrapper):
            size = self.realSize(obj)
            self.incrementByteCount('realBytes', incr=1+size)
        elif isinstance(obj, datetime.datetime):    
            self.incrementByteCount('dateBytes', incr=2)
        elif isinstance(obj, Data):
            size = proc_size(len(obj))
            self.incrementByteCount('dataBytes', incr=1+size)
        elif isinstance(obj, StringWrapper):
            size = proc_size(len(obj))
            self.incrementByteCount('stringBytes', incr=1+size)
        elif isinstance(obj, HashableWrapper):
            obj = obj.value
            if isinstance(obj, set):
                size = proc_size(len(obj))
                self.incrementByteCount('setBytes', incr=1+size)
                for value in obj:
                    self.computeOffsets(value, asReference=True)
            elif isinstance(obj, (list, tuple)):
                size = proc_size(len(obj))
                self.incrementByteCount('arrayBytes', incr=1+size)
                for value in obj:
                    asRef = True
                    self.computeOffsets(value, asReference=True)
            elif isinstance(obj, dict):
                size = proc_size(len(obj))
                self.incrementByteCount('dictBytes', incr=1+size)
                for key, value in iteritems(obj):
                    check_key(key)
                    self.computeOffsets(key, asReference=True)
                    self.computeOffsets(value, asReference=True)
        else:
            raise InvalidPlistException("Unknown object type: %s (%s)" % (type(obj).__name__, repr(obj)))

    def writeObjectReference(self, obj, output):
        """Tries to write an object reference, adding it to the references
           table. Does not write the actual object bytes or set the reference
           position. Returns a tuple of whether the object was a new reference
           (True if it was, False if it already was in the reference table)
           and the new output.
        """
        position = self.positionOfObjectReference(obj)
        if position is None:
            self.writtenReferences[obj] = len(self.writtenReferences)
            output += self.binaryInt(len(self.writtenReferences) - 1, byteSize=self.trailer.objectRefSize)
            return (True, output)
        else:
            output += self.binaryInt(position, byteSize=self.trailer.objectRefSize)
            return (False, output)

    def writeObject(self, obj, output, setReferencePosition=False):
        """Serializes the given object to the output. Returns output.
           If setReferencePosition is True, will set the position the
           object was written.
        """
        def proc_variable_length(format, length):
            result = b''
            if length > 0b1110:
                result += pack('!B', (format << 4) | 0b1111)
                result = self.writeObject(length, result)
            else:
                result += pack('!B', (format << 4) | length)
            return result
        
        def timedelta_total_seconds(td):
            # Shim for Python 2.6 compatibility, which doesn't have total_seconds.
            # Make one argument a float to ensure the right calculation.
            return (td.microseconds + (td.seconds + td.days * 24 * 3600) * 10.0**6) / 10.0**6
       
        if setReferencePosition:
            self.referencePositions[obj] = len(output)
        
        if obj is None:
            output += pack('!B', 0b00000000)
        elif isinstance(obj, BoolWrapper):
            if obj.value is False:
                output += pack('!B', 0b00001000)
            else:
                output += pack('!B', 0b00001001)
        elif isinstance(obj, Uid):
            size = self.intSize(obj.integer)
            output += pack('!B', (0b1000 << 4) | size - 1)
            output += self.binaryInt(obj.integer)
        elif isinstance(obj, (int, long)):
            byteSize = self.intSize(obj)
            root = math.log(byteSize, 2)
            output += pack('!B', (0b0001 << 4) | int(root))
            output += self.binaryInt(obj, as_number=True)
        elif isinstance(obj, FloatWrapper):
            # just use doubles
            output += pack('!B', (0b0010 << 4) | 3)
            output += self.binaryReal(obj)
        elif isinstance(obj, datetime.datetime):
            try:
                timestamp = (obj - apple_reference_date).total_seconds()
            except AttributeError:
                timestamp = timedelta_total_seconds(obj - apple_reference_date)
            output += pack('!B', 0b00110011)
            output += pack('!d', float(timestamp))
        elif isinstance(obj, Data):
            output += proc_variable_length(0b0100, len(obj))
            output += obj
        elif isinstance(obj, StringWrapper):
            output += proc_variable_length(obj.encodingMarker, len(obj))
            output += obj.encodedValue
        elif isinstance(obj, bytes):
            output += proc_variable_length(0b0101, len(obj))
            output += obj
        elif isinstance(obj, HashableWrapper):
            obj = obj.value
            if isinstance(obj, (set, list, tuple)):
                if isinstance(obj, set):
                    output += proc_variable_length(0b1100, len(obj))
                else:
                    output += proc_variable_length(0b1010, len(obj))
            
                objectsToWrite = []
                for objRef in obj:
                    (isNew, output) = self.writeObjectReference(objRef, output)
                    if isNew:
                        objectsToWrite.append(objRef)
                for objRef in objectsToWrite:
                    output = self.writeObject(objRef, output, setReferencePosition=True)
            elif isinstance(obj, dict):
                output += proc_variable_length(0b1101, len(obj))
                keys = []
                values = []
                objectsToWrite = []
                for key, value in iteritems(obj):
                    keys.append(key)
                    values.append(value)
                for key in keys:
                    (isNew, output) = self.writeObjectReference(key, output)
                    if isNew:
                        objectsToWrite.append(key)
                for value in values:
                    (isNew, output) = self.writeObjectReference(value, output)
                    if isNew:
                        objectsToWrite.append(value)
                for objRef in objectsToWrite:
                    output = self.writeObject(objRef, output, setReferencePosition=True)
        return output
    
    def writeOffsetTable(self, output):
        """Writes all of the object reference offsets."""
        all_positions = []
        writtenReferences = list(self.writtenReferences.items())
        writtenReferences.sort(key=lambda x: x[1])
        for obj,order in writtenReferences:
            # Porting note: Elsewhere we deliberately replace empty unicdoe strings
            # with empty binary strings, but the empty unicode string
            # goes into writtenReferences.  This isn't an issue in Py2
            # because u'' and b'' have the same hash; but it is in
            # Py3, where they don't.
            if bytes != str and obj == unicodeEmpty:
                obj = b''
            position = self.referencePositions.get(obj)
            if position is None:
                raise InvalidPlistException("Error while writing offsets table. Object not found. %s" % obj)
            output += self.binaryInt(position, self.trailer.offsetSize)
            all_positions.append(position)
        return output
    
    def binaryReal(self, obj):
        # just use doubles
        result = pack('>d', obj.value)
        return result
    
    def binaryInt(self, obj, byteSize=None, as_number=False):
        result = b''
        if byteSize is None:
            byteSize = self.intSize(obj)
        if byteSize == 1:
            result += pack('>B', obj)
        elif byteSize == 2:
            result += pack('>H', obj)
        elif byteSize == 4:
            result += pack('>L', obj)
        elif byteSize == 8:
            if as_number:
                result += pack('>q', obj)
            else:
                result += pack('>Q', obj)
        elif byteSize <= 16:
            try:
                result = pack('>Q', 0) + pack('>Q', obj)
            except struct_error as e:
                raise InvalidPlistException("Unable to pack integer %d: %s" % (obj, e))
        else:
            raise InvalidPlistException("Core Foundation can't handle integers with size greater than 16 bytes.")
        return result
    
    def intSize(self, obj):
        """Returns the number of bytes necessary to store the given integer."""
        # SIGNED
        if obj < 0: # Signed integer, always 8 bytes
            return 8
        # UNSIGNED
        elif obj <= 0xFF: # 1 byte
            return 1
        elif obj <= 0xFFFF: # 2 bytes
            return 2
        elif obj <= 0xFFFFFFFF: # 4 bytes
            return 4
        # SIGNED
        # 0x7FFFFFFFFFFFFFFF is the max.
        elif obj <= 0x7FFFFFFFFFFFFFFF: # 8 bytes signed
            return 8
        elif obj <= 0xffffffffffffffff: # 8 bytes unsigned
            return 16
        else:
            raise InvalidPlistException("Core Foundation can't handle integers with size greater than 8 bytes.")
    
    def realSize(self, obj):
        return 8
