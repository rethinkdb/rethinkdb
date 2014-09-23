# -*- coding: utf-8 -*-
import os
import bisect
import struct
import binascii

try:
    {}.iterkeys
    iterkeys = lambda x: x.iterkeys()
except AttributeError:
    iterkeys = lambda x: x.keys()
try:
    unicode
except NameError:
    unicode = str

class BuddyError(Exception):
    pass

class Block(object):
    def __init__(self, allocator, offset, size):
        self._allocator = allocator
        self._offset = offset
        self._size = size
        self._value = bytearray(allocator.read(offset, size))
        self._pos = 0
        self._dirty = False
        
    def __len__(self):
        return self._size

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
    
    def close(self):
        if self._dirty:
            self.flush()

    def flush(self):
        if self._dirty:
            self._dirty = False
            self._allocator.write(self._offset, self._value)

    def invalidate(self):
        self._dirty = False
        
    def zero_fill(self):
        len = self._size - self._pos
        zeroes = b'\0' * len
        self._value[self._pos:self._size] = zeroes
        self._dirty = True
        
    def tell(self):
        return self._pos

    def seek(self, pos, whence=os.SEEK_SET):
        if whence == os.SEEK_CUR:
            pos += self._pos
        elif whence == os.SEEK_END:
            pos = self._size - pos

        if pos < 0 or pos > self._size:
            raise ValueError('Seek out of range in Block instance')

        self._pos = pos

    def read(self, size_or_format):
        if isinstance(size_or_format, (str, unicode, bytes)):
            size = struct.calcsize(size_or_format)
            fmt = size_or_format
        else:
            size = size_or_format
            fmt = None

        if self._size - self._pos < size:
            raise BuddyError('Unable to read %lu bytes in block' % size)

        data = self._value[self._pos:self._pos + size]
        self._pos += size
        
        if fmt is not None:
            if isinstance(data, bytearray):
                return struct.unpack_from(fmt, bytes(data))
            else:
                return struct.unpack(fmt, data)
        else:
            return data

    def write(self, data_or_format, *args):
        if len(args):
            data = struct.pack(data_or_format, *args)
        else:
            data = data_or_format

        if self._pos + len(data) > self._size:
            raise ValueError('Attempt to write past end of Block')

        self._value[self._pos:self._pos + len(data)] = data
        self._pos += len(data)
        
        self._dirty = True

    def insert(self, data_or_format, *args):
        if len(args):
            data = struct.pack(data_or_format, *args)
        else:
            data = data_or_format

        del self._value[-len(data):]
        self._value[self._pos:self._pos] = data
        self._pos += len(data)

        self._dirty = True

    def delete(self, size):
        if self._pos + size > self._size:
            raise ValueError('Attempt to delete past end of Block')
        del self._value[self._pos:self._pos + size]
        self._value += b'\0' * size
        self._dirty = True
        
    def __str__(self):
        return binascii.b2a_hex(self._value)
        
class Allocator(object):
    def __init__(self, the_file):
        self._file = the_file
        self._dirty = False

        self._file.seek(0)
        
        # Read the header
        magic1, magic2, offset, size, offset2, self._unknown1 \
          = self.read(-4, '>I4sIII16s')

        if magic2 != b'Bud1' or magic1 != 1:
            raise BuddyError('Not a buddy file')

        if offset != offset2:
            raise BuddyError('Root addresses differ')

        self._root = Block(self, offset, size)

        # Read the block offsets
        count, self._unknown2 = self._root.read('>II')
        self._offsets = []
        c = (count + 255) & ~255
        while c:
            self._offsets += self._root.read('>256I')
            c -= 256
        self._offsets = self._offsets[:count]
        
        # Read the TOC
        self._toc = {}
        count = self._root.read('>I')[0]
        for n in range(count):
            nlen = self._root.read('B')[0]
            name = str(self._root.read(nlen))
            value = self._root.read('>I')[0]
            self._toc[name] = value

        # Read the free lists
        self._free = []
        for n in range(32):
            count = self._root.read('>I')
            self._free.append(list(self._root.read('>%uI' % count)))
        
    @classmethod
    def open(cls, file_or_name, mode='r+'):
        if isinstance(file_or_name, (str, unicode)):
            if not 'b' in mode:
                mode = mode[:1] + 'b' + mode[1:]
            f = open(file_or_name, mode)
        else:
            f = file_or_name

        if 'w' in mode:
            # Create an empty file in this case
            f.truncate()
            
            # An empty root block needs 1264 bytes:
            #
            #     0  4       offset count
            #     4  4       unknown
            #     8  4       root block offset (2048)
            #    12  255 * 4 padding (offsets are in multiples of 256)
            #  1032  4       toc count (0)
            #  1036  228     free list
            #  total 1264
            
            # The free list will contain the following:
            #
            #     0  5 * 4   no blocks of width less than 5
            #    20  6 * 8   1 block each of widths 5 to 10
            #    68  4       no blocks of width 11 (allocated for the root)
            #    72  19 * 8  1 block each of widths 12 to 30
            #   224  4       no blocks of width 31
            # total  228
            #
            # (The reason for this layout is that we allocate 2**5 bytes for
            #  the header, which splits the initial 2GB region into every size
            #  below 2**31, including *two* blocks of size 2**5, one of which
            #  we take.  The root block itself then needs a block of size
            #  2**11.  Conveniently, each of these initial blocks will be
            #  located at offset 2**n where n is its width.)

            # Write the header
            header = struct.pack(b'>I4sIII16s',
                                 1, b'Bud1',
                                 2048, 1264, 2048,
                                 b'\x00\x00\x10\x0c'
                                 b'\x00\x00\x00\x87'
                                 b'\x00\x00\x20\x0b'
                                 b'\x00\x00\x00\x00')
            f.write(header)
            f.write(b'\0' * 2016)
            
            # Write the root block
            free_list = [struct.pack(b'>5I', 0, 0, 0, 0, 0)]
            for n in range(5, 11):
                free_list.append(struct.pack(b'>II', 1, 2**n))
            free_list.append(struct.pack(b'>I', 0))
            for n in range(12, 31):
                free_list.append(struct.pack(b'>II', 1, 2**n))
            free_list.append(struct.pack(b'>I', 0))
            
            root = b''.join([struct.pack(b'>III', 1, 0, 2048 | 5),
                            struct.pack(b'>I', 0) * 255,
                            struct.pack(b'>I', 0)] + free_list)
            f.write(root)

        return Allocator(f)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
    
    def close(self):
        self.flush()
        self._file.close()

    def flush(self):
        if self._dirty:
            size = self._root_block_size()
            self.allocate(size, 0)
            with self.get_block(0) as rblk:
                self._write_root_block_into(rblk)

            addr = self._offsets[0]
            offset = addr & ~0x1f
            size = 1 << (addr & 0x1f)

            self._file.seek(0, os.SEEK_SET)
            self._file.write(struct.pack(b'>I4sIII16s',
                                         1, b'Bud1',
                                         offset, size, offset,
                                         self._unknown1))

            self._dirty = False
            
        self._file.flush()
            
    def read(self, offset, size_or_format):
        """Read data at `offset', or raise an exception.  `size_or_format'
           may either be a byte count, in which case we return raw data,
           or a format string for `struct.unpack', in which case we
           work out the size and unpack the data before returning it."""
        # N.B. There is a fixed offset of four bytes(!)
        self._file.seek(offset + 4, os.SEEK_SET)

        if isinstance(size_or_format, (str, unicode)):
            size = struct.calcsize(size_or_format)
            fmt = size_or_format
        else:
            size = size_or_format
            fmt = None
        
        ret = self._file.read(size)
        if len(ret) < size:
            ret += b'\0' * (size - len(ret))

        if fmt is not None:
            if isinstance(ret, bytearray):
                ret = struct.unpack_from(fmt, bytes(ret))
            else:
                ret = struct.unpack(fmt, ret)
            
        return ret

    def write(self, offset, data_or_format, *args):
        """Write data at `offset', or raise an exception.  `data_or_format'
           may either be the data to write, or a format string for `struct.pack',
           in which case we pack the additional arguments and write the
           resulting data."""
        # N.B. There is a fixed offset of four bytes(!)
        self._file.seek(offset + 4, os.SEEK_SET)

        if len(args):
            data = struct.pack(data_or_format, *args)
        else:
            data = data_or_format

        self._file.write(data)

    def get_block(self, block):
        try:
            addr = self._offsets[block]
        except IndexError:
            return None

        offset = addr & ~0x1f
        size = 1 << (addr & 0x1f)

        return Block(self, offset, size)
    
    def _root_block_size(self):
        """Return the number of bytes required by the root block."""
        # Offsets
        size = 8
        size += 4 * ((len(self._offsets) + 255) & ~255)

        # TOC
        size += 4
        size += sum([5 + len(s) for s in self._toc])

        # Free list
        size += sum([4 + 4 * len(fl) for fl in self._free])
        
        return size

    def _write_root_block_into(self, block):
        # Offsets
        block.write('>II', len(self._offsets), self._unknown2)
        block.write('>%uI' % len(self._offsets), *self._offsets)
        extra = len(self._offsets) & 255
        if extra:
            block.write(b'\0\0\0\0' * (256 - extra))

        # TOC
        keys = list(self._toc.keys())
        keys.sort()

        block.write('>I', len(keys))
        for k in keys:
            b = k.encode('utf-8')
            block.write('B', len(b))
            block.write(b)
            block.write('>I', self._toc[k])

        # Free list
        for w, f in enumerate(self._free):
            block.write('>I', len(f))
            if len(f):
                block.write('>%uI' % len(f), *f)

    def _buddy(self, offset, width):
        f = self._free[width]
        b = offset ^ (1 << width)
        
        try:
            ndx = f.index(b)
        except ValueError:
            ndx = None
            
        return (f, b, ndx)

    def _release(self, offset, width):
        # Coalesce
        while True:
            f,b,ndx = self._buddy(offset, width)

            if ndx is None:
                break
            
            offset &= b
            width += 1
            del f[ndx]
            
        # Add to the list
        bisect.insort(f, offset)

        # Mark as dirty
        self._dirty = True
    
    def _alloc(self, width):
        w = width
        while not self._free[w]:
            w += 1
        while w > width:
            offset = self._free[w].pop(0)
            w -= 1
            self._free[w] = [offset, offset ^ (1 << w)]
        self._dirty = True
        return self._free[width].pop(0)

    def allocate(self, bytes, block=None):
        """Allocate or reallocate a block such that it has space for at least
        `bytes' bytes."""
        if block is None:
            # Find the first unused block
            try:
                block = self._offsets.index(0)
            except ValueError:
                block = len(self._offsets)
                self._offsets.append(0)
        
        # Compute block width
        width = max(bytes.bit_length(), 5)

        addr = self._offsets[block]
        offset = addr & ~0x1f
        
        if addr:
            blkwidth = addr & 0x1f
            if blkwidth == width:
                return block
            self._release(offset, width)
            self._offsets[block] = 0

        offset = self._alloc(width)
        self._offsets[block] = offset | width
        return block
    
    def release(self, block):
        addr = self._offsets[block]

        if addr:
            width = addr & 0x1f
            offset = addr & ~0x1f
            self._release(offset, width)

        if block == len(self._offsets):
            del self._offsets[block]
        else:
            self._offsets[block] = 0

    def __len__(self):
        return len(self._toc)

    def __getitem__(self, key):
        if not isinstance(key, (str, unicode)):
            raise TypeError('Keys must be of string type')
        return self._toc[key]

    def __setitem__(self, key, value):
        if not isinstance(key, (str, unicode)):
            raise TypeError('Keys must be of string type')
        self._toc[key] = value
        self._dirty = True

    def __delitem__(self, key):
        if not isinstance(key, (str, unicode)):
            raise TypeError('Keys must be of string type')
        del self._toc[key]
        self._dirty = True

    def iterkeys(self):
        return iterkeys(self._toc)

    def keys(self):
        return iterkeys(self._toc)
        
    def __iter__(self):
        return iterkeys(self._toc)

    def __contains__(self, key):
        return key in self._toc

