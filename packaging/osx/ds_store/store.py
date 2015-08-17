# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from __future__ import print_function
from __future__ import division

import binascii
import struct
import biplist

try:
    next
except NameError:
    next = lambda x: x.next()
try:
    unicode
except NameError:
    unicode = str

from . import buddy

class ILocCodec(object):
    @staticmethod
    def encode(point):
        return struct.pack(b'>IIII', point[0], point[1],
                           0xffffffff, 0xffff0000)

    @staticmethod
    def decode(bytesData):
        if isinstance(bytesData, bytearray):
            x, y = struct.unpack_from(b'>II', bytes(bytesData[:8]))
        else:
            x, y = struct.unpack(b'>II', bytesData[:8])
        return (x, y)

class PlistCodec(object):
    @staticmethod
    def encode(plist):
        return biplist.writePlistToString(plist)

    @staticmethod
    def decode(bytes):
        return biplist.readPlistFromString(bytes)

# This list tells the code how to decode particular kinds of entry in the
# .DS_Store file.  This is really a convenience, and we currently only
# support a tiny subset of the possible entry types.
codecs = {
    'Iloc': ILocCodec,
    'bwsp': PlistCodec,
    'lsvp': PlistCodec,
    'lsvP': PlistCodec,
    'icvp': PlistCodec,
    }

class DSStoreEntry(object):
    """Holds the data from an entry in a ``.DS_Store`` file.  Note that this is
    not meant to represent the entry itself---i.e. if you change the type
    or value, your changes will *not* be reflected in the underlying file.

    If you want to make a change, you should either use the :class:`DSStore`
    object's :meth:`DSStore.insert` method (which will replace a key if it
    already exists), or the mapping access mode for :class:`DSStore` (often
    simpler anyway).
    """
    def __init__(self, filename, code, typecode, value=None):
        if str != bytes and type(filename) == bytes:
            filename = filename.decode('utf-8')
        self.filename = filename
        self.code = code
        self.type = typecode
        self.value = value
        
    @classmethod
    def read(cls, block):
        """Read a ``.DS_Store`` entry from the containing Block"""
        # First read the filename
        nlen = block.read(b'>I')[0]
        filename = block.read(2 * nlen).decode('utf-16be')

        # Next, read the code and type
        code, typecode = block.read(b'>4s4s')

        # Finally, read the data
        if typecode == b'bool':
            value = block.read(b'>?')[0]
        elif typecode == b'long' or typecode == b'shor':
            value = block.read(b'>I')[0]
        elif typecode == b'blob':
            vlen = block.read(b'>I')[0]
            value = block.read(vlen)

            codec = codecs.get(code, None)
            if codec:
                value = codec.decode(value)
                typecode = codec
        elif typecode == b'ustr':
            vlen = block.read(b'>I')[0]
            value = block.read(2 * vlen).decode('utf-16be')
        elif typecode == b'type':
            value = block.read(b'>4s')[0]
        elif typecode == b'comp' or typecode == b'dutc':
            value = block.read(b'>Q')[0]
        else:
            raise ValueError('Unknown type code "%s"' % typecode)

        return DSStoreEntry(filename, code, typecode, value)

    def __lt__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        return (sfl < ofl
                or (self.filename == other.filename
                    and self.code < other.code))

    def __le__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        return (sfl < ofl
                or (sfl == ofl
                    and self.code <= other.code))

    def __eq__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        return (sfl == ofl
                and self.code == other.code)

    def __ne__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        return (sfl != ofl
                or self.code != other.code)

    def __gt__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        
        selfCode = self.code
        if str != bytes and type(selfCode) is bytes:
            selfCode = selfCode.decode('utf-8')
        otherCode = other.code
        if str != bytes and type(otherCode) is bytes:
            otherCode = otherCode.decode('utf-8')
        
        return (sfl > ofl or (sfl == ofl and selfCode > otherCode))

    def __ge__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        sfl = self.filename.lower()
        ofl = other.filename.lower()
        return (sfl > ofl
                or (sfl == ofl
                    and self.code >= other.code))

    def __cmp__(self, other):
        if not isinstance(other, DSStoreEntry):
            raise TypeError('Can only compare against other DSStoreEntry objects')
        r = cmp(self.filename.lower(), other.filename.lower())
        if r:
            return r
        return cmp(self.code, other.code)
    
    def byte_length(self):
        """Compute the length of this entry, in bytes"""
        utf16 = self.filename.encode('utf-16be')
        l = 4 + len(utf16) + 8

        if isinstance(self.type, (str, unicode)):
            entry_type = self.type
            value = self.value
        else:
            entry_type = 'blob'
            value = self.type.encode(self.value)
            
        if entry_type == 'bool':
            l += 1
        elif entry_type == 'long' or entry_type == 'shor':
            l += 4
        elif entry_type == 'blob':
            l += 4 + len(value)
        elif entry_type == 'ustr':
            utf16 = value.encode('utf-16be')
            l += 4 + len(utf16)
        elif entry_type == 'type':
            l += 4
        elif entry_type == 'comp' or entry_type == 'dutc':
            l += 8
        else:
            raise ValueError('Unknown type code "%s"' % entry_type)

        return l
    
    def write(self, block, insert=False):
        """Write this entry to the specified Block"""
        if insert:
            w = block.insert
        else:
            w = block.write
        
        if isinstance(self.type, (str, unicode)):
            entry_type = self.type
            value = self.value
        else:
            entry_type = 'blob'
            value = self.type.encode(self.value)
            
        utf16 = self.filename.encode('utf-16be')
        w(b'>I', len(utf16) // 2)
        w(utf16)
        w(b'>4s4s', self.code.encode('utf-8'), entry_type.encode('utf-8'))

        if entry_type == 'bool':
            w(b'>?', value)
        elif entry_type == 'long' or entry_type == 'shor':
            w(b'>I', value)
        elif entry_type == 'blob':
            w(b'>I', len(value))
            w(value)
        elif entry_type == 'ustr':
            utf16 = value.encode('utf-16be')
            w(b'>I', len(utf16) // 2)
            w(utf16)
        elif entry_type == 'type':
            w(b'>4s', value.encode('utf-8'))
        elif entry_type == 'comp' or entry_type == 'dutc':
            w(b'>Q', value)
        else:
            raise ValueError('Unknown type code "%s"' % entry_type)
    
    def __repr__(self):
        return '<%s %s>' % (self.filename, self.code)

class DSStore(object):
    """Python interface to a ``.DS_Store`` file.  Works by manipulating the file
    on the disk---so this code will work with ``.DS_Store`` files for *very*
    large directories.

    A :class:`DSStore` object can be used as if it was a mapping, e.g.::

      d['foobar.dat']['Iloc']

    will fetch the "Iloc" record for "foobar.dat", or raise :class:`KeyError` if
    there is no such record.  If used in this manner, the :class:`DSStore` object
    will return (type, value) tuples, unless the type is "blob" and the module
    knows how to decode it.

    Currently, we know how to decode "Iloc", "bwsp", "lsvp", "lsvP" and "icvp"
    blobs.  "Iloc" decodes to an (x, y) tuple, while the others are all decoded
    using ``biplist``.

    Assignment also works, e.g.::

      d['foobar.dat']['note'] = ('ustr', u'Hello World!')

    as does deletion with ``del``::

      del d['foobar.dat']['note']

    This is usually going to be the most convenient interface, though
    occasionally (for instance when creating a new ``.DS_Store`` file) you
    may wish to drop down to using :class:`DSStoreEntry` objects directly."""
    def __init__(self, store):
        self._store = store
        self._superblk = self._store['DSDB']
        with self._get_block(self._superblk) as s:
            self._rootnode, self._levels, self._records, \
            self._nodes, self._page_size = s.read(b'>IIIII')
        self._min_usage = 2 * self._page_size // 3
        self._dirty = False
        
    @classmethod
    def open(cls, file_or_name, mode='r+', initial_entries=None):
        """Open a ``.DS_Store`` file; pass either a Python file object, or a
        filename in the ``file_or_name`` argument and a file access mode in
        the ``mode`` argument.  If you are creating a new file using the "w"
        or "w+" modes, you may also specify a list of entries with which
        to initialise the file."""
        store = buddy.Allocator.open(file_or_name, mode)
        
        if mode == 'w' or mode == 'w+':
            superblk = store.allocate(20)
            store['DSDB'] = superblk
            page_size = 4096
            
            if not initial_entries:
                root = store.allocate(page_size)
                
                with store.get_block(root) as rootblk:
                    rootblk.zero_fill()

                with store.get_block(superblk) as s:
                    s.write(b'>IIIII', root, 0, 0, 1, page_size)
            else:
                # Make sure they're in sorted order
                initial_entries = list(initial_entries)
                initial_entries.sort()

                # Construct the tree
                current_level = initial_entries
                next_level = []
                levels = []
                ptr_size = 0
                node_count = 0
                while True:
                    total = 8
                    nodes = []
                    node = []
                    for e in current_level:
                        new_total = total + ptr_size + e.byte_length()
                        if new_total > page_size:
                            nodes.append(node)
                            next_level.append(e)
                            total = 8
                            node = []
                        else:
                            total = new_total
                            node.append(e)
                    if node:
                        nodes.append(node)

                    node_count += len(nodes)
                    levels.append(nodes)

                    if len(nodes) == 1:
                        break
                    
                    current_level = next_level
                    next_level = []
                    ptr_size = 4

                # Allocate nodes
                ptrs = [store.allocate(page_size) for n in range(node_count)]
                
                # Generate nodes
                pointers = []
                prev_pointers = None
                for level in levels:
                    ppndx = 0
                    lptrs = ptrs[-len(level):]
                    del ptrs[-len(level):]
                    for node in level:
                        ndx = lptrs.pop(0)
                        if prev_pointers is None:
                            with store.get_block(ndx) as block:
                                block.write(b'>II', 0, len(node))
                                for e in node:
                                    e.write(block)
                        else:
                            next_node = prev_pointers[ppndx + len(node)]
                            node_ptrs = prev_pointers[ppndx:ppndx+len(node)]
                            
                            with store.get_block(ndx) as block:
                                block.write(b'>II', next_node, len(node))
                                for ptr, e in zip(node_ptrs, node):
                                    block.write(b'>I', ptr)
                                    e.write(block)
                            
                        pointers.append(ndx)
                    prev_pointers = pointers
                    pointers = []
                
                root = prev_pointers[0]

                with store.get_block(superblk) as s:
                    s.write(b'>IIIII', root, len(levels), len(initial_entries),
                            node_count, page_size)
                    
        return DSStore(store)

    def _get_block(self, number):
        return self._store.get_block(number)

    def flush(self):
        """Flush any dirty data back to the file."""
        if self._dirty:
            self._dirty = False

            with self._get_block(self._superblk) as s:
                s.write(b'>IIIII', self._rootnode, self._levels, self._records,
                        self._nodes, self._page_size)
        self._store.flush()

    def close(self):
        """Flush dirty data and close the underlying file."""
        self.flush()
        self._store.close()
        
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        
    # Internal B-Tree nodes look like this:
    #
    # [ next | count | (ptr0 | rec0) | (ptr1 | rec1) ... (ptrN | recN) ]
    
    # Leaf nodes look like this:
    #
    # [ 0 | count | rec0 | rec1 ... recN ]

    # Iterate over the tree, starting at `node'
    def _traverse(self, node):
        if node is None:
            node = self._rootnode
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            if next_node:
                for n in range(count):
                    ptr = block.read(b'>I')[0]
                    for e in self._traverse(ptr):
                        yield e
                    e = DSStoreEntry.read(block)
                    yield e
                for e in self._traverse(next_node):
                    yield e
            else:
                for n in range(count):
                    e = DSStoreEntry.read(block)
                    yield e

    # Display the data in `node'
    def _dump_node(self, node):
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            print('next: %u\ncount: %u\n' % (next_node, count))
            for n in range(count):
                if next_node:
                    ptr = block.read(b'>I')[0]
                    print('%8u ' % ptr, end=' ')
                else:
                    print('         ', end=' ')
                e = DSStoreEntry.read(block)
                print(e, ' (%u)' % e.byte_length())
            print('used: %u' % block.tell())

    # Display the data in the super block
    def _dump_super(self):
        print('root: %u\nlevels: %u\nrecords: %u\nnodes: %u\npage-size: %u' \
          % (self._rootnode, self._levels, self._records,
             self._nodes, self._page_size))

    # Splits entries across two blocks, returning one pivot
    #
    # Tries to balance the block usage across the two as best it can
    def _split2(self, blocks, entries, pointers, before, internal):
        left_block = blocks[0]
        right_block = blocks[1]
        
        count = len(entries)
        
        # Find the feasible splits
        best_split = None
        best_diff = None
        total = before[count]

        if 8 + total <= self._page_size:
            # We can use a *single* node for this
            best_split = count
        else:
            # Split into two nodes  
            for n in range(1, count - 1):
                left_size = 8 + before[n]
                right_size = 8 + total - before[n + 1]

                if left_size > self._page_size:
                    break
                if right_size > self._page_size:
                    continue
                
                diff = abs(left_size - right_size)

                if best_split is None or diff < best_diff:
                    best_split = n
                    best_diff = diff

        if best_split is None:
            return None
        
        # Write the nodes
        left_block.seek(0)
        if internal:
            next_node = pointers[best_split]
        else:
            next_node = 0
        left_block.write(b'>II', next_node, best_split)

        for n in range(best_split):
            if internal:
                left_block.write(b'>I', pointers[n])
            entries[n].write(left_block)

        left_block.zero_fill()

        if best_split == count:
            return []
        
        right_block.seek(0)
        if internal:
            next_node = pointers[count]
        else:
            next_node = 0
        right_block.write(b'>II', next_node, count - best_split - 1)

        for n in range(best_split + 1, count):
            if internal:
                right_block.write(b'>I', pointers[n])
            entries[n].write(right_block)

        right_block.zero_fill()
            
        pivot = entries[best_split]

        return [pivot]
    
    def _split(self, node, entry, right_ptr=0):
        self._nodes += 1
        self._dirty = True
        new_right = self._store.allocate(self._page_size)
        with self._get_block(node) as block, \
            self._get_block(new_right) as right_block:

            # First, measure and extract all the elements
            entry_size = entry.byte_length()
            entry_pos = None
            next_node, count = block.read(b'>II')
            if next_node:
                entry_size += 4
            pointers = []
            entries = []
            before = []
            total = 0
            for n in range(count):
                pos = block.tell()
                if next_node:
                    ptr = block.read(b'>I')[0]
                    pointers.append(ptr)
                e = DSStoreEntry.read(block)
                if e > entry:
                    entry_pos = n
                    entries.append(entry)
                    pointers.append(right_ptr)
                    before.append(total)
                    total += entry_Size
                entries.append(e)
                before.append(total)
                total += block.tell() - pos
            before.append(total)
            if next_node:
                pointers.append(next_node)

            pivot = self._split2([left_block, right_block],
                                 entries, pointers, before,
                                 bool(next_node))[0]
            
            self._records += 1
            self._nodes += 1
            self._dirty = True

        return (pivot, new_right)

    # Allocate a new root node containing the element `pivot' and the pointers
    # `left' and `right'
    def _new_root(self, left, pivot, right):
        new_root = self._store.allocate(self._page_size)
        with self._get_block(new_root) as block:
            block.write(b'>III', right, 1, left)
            pivot.write(block)
        self._rootnode = new_root
        self._levels += 1
        self._nodes += 1
        self._dirty = True

    # Insert an entry into an inner node; `path' is the path from the root
    # to `node', not including `node' itself.  `right_ptr' is the new node
    # pointer (inserted to the RIGHT of `entry')
    def _insert_inner(self, path, node, entry, right_ptr):
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            insert_pos = None
            insert_ndx = None
            n = 0
            while n < count:
                pos = block.tell()
                ptr = block.read(b'>I')[0]
                e = DSStoreEntry.read(block)
                if e == entry:
                    if n == count - 1:
                        right_ptr = next_node
                        next_node = ptr
                        block_seek(pos)
                    else:
                        right_ptr = block.read(b'>I')[0]
                        block.seek(pos + 4)
                    insert_pos = pos
                    insert_ndx = n
                    block.delete(e.byte_length() + 4)
                    count -= 1
                    self._records += 1
                    self._dirty = True
                    continue
                elif insert_pos is None and e > entry:
                    insert_pos = pos
                    insert_ndx = n
                n += 1
            if insert_pos is None:
                insert_pos = block.tell()
                insert_ndx = count
            remaining = self._page_size - block.tell()

            if remaining < entry.byte_length() + 4:
                pivot, new_right = self._split(node, entry, right_ptr)
                if path:
                    self._insert_inner(path[:-1], path[-1], pivot, new_right)
                else:
                    self._new_root(node, pivot, new_right)
            else:
                if insert_ndx == count:
                    block.seek(insert_pos)
                    block.write(b'>I', next_node)
                    entry.write(block)
                    next_node = right_ptr
                else:
                    block.seek(insert_pos + 4)
                    entry.write(block, True)
                    block.insert('>I', right_ptr)
                block.seek(0)
                count += 1
                block.write(b'>II', next_node, count)
                self._records += 1
                self._dirty = True

    # Insert `entry' into the leaf node `node'
    def _insert_leaf(self, path, node, entry):
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            insert_pos = None
            insert_ndx = None
            n = 0
            while n < count:
                pos = block.tell()
                e = DSStoreEntry.read(block)
                if e == entry:
                    insert_pos = pos
                    insert_ndx = n
                    block.seek(pos)
                    block.delete(e.byte_length())
                    count -= 1
                    self._records += 1
                    self._dirty = True
                    continue
                elif insert_pos is None and e > entry:
                    insert_pos = pos
                    insert_ndx = n
                n += 1
            if insert_pos is None:
                insert_pos = block.tell()
                insert_ndx = count
            remaining = self._page_size - block.tell()

            if remaining < entry.byte_length():
                pivot, new_right = self._split(node, entry)
                if path:
                    self._insert_inner(path[:-1], path[-1], pivot, new_right)
                else:
                    self._new_root(node, pivot, new_right)
            else:
                block.seek(insert_pos)
                entry.write(block, True)
                block.seek(0)
                count += 1
                block.write(b'>II', next_node, count)
                self._records += 1
                self._dirty = True

    def insert(self, entry):
        """Insert ``entry`` (which should be a :class:`DSStoreEntry`)
        into the B-Tree."""
        path = []
        node = self._rootnode
        while True:
            with self._get_block(node) as block:
                next_node, count = block.read(b'>II')
                if next_node:
                    for n in range(count):
                        ptr = block.read(b'>I')[0]
                        e = DSStoreEntry.read(block)
                        if entry < e:
                            next_node = ptr
                            break
                        elif entry == e:
                            # If we find an existing entry the same, replace it
                            self._insert_inner(path, node, entry, None)
                            return
                    path.append(node)
                    node = next_node
                else:
                    self._insert_leaf(path, node, entry)
                    return

    # Return usage information for the specified `node'
    def _block_usage(self, node):
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')

            for n in range(count):
                if next_node:
                    ptr = block.read(b'>I')[0]
                e = DSStoreEntry.read(block)

            used = block.tell()

        return (count, used)

    # Splits entries across three blocks, returning two pivots
    def _split3(self, blocks, entries, pointers, before, internal):
        count = len(entries)

        # Find the feasible splits
        best_split = None
        best_diff = None
        total = before[count]
        for n in range(1, count - 3):
            left_size = 8 + before[n]
            remaining = 16 + total - before[n + 1]

            if left_size > self._page_size:
                break
            if remaining > 2 * self._page_size:
                continue

            for m in range(n + 2, count - 1):
                mid_size = 8 + before[m] - before[n + 1]
                right_size = 8 + total - before[m + 1]

                if mid_size > self._page_size:
                    break
                if right_size > self._page_size:
                    continue

                diff = abs(left_size - mid_size) * abs(right_size - mid_size)
                
                if best_split is None or diff < best_diff:
                    best_split = (n, m, count)
                    best_diff = diff

        if best_split is None:
            return None
        
        # Write the nodes
        prev_split = -1
        for block, split in zip(blocks, best_split):
            block.seek(0)
            if internal:
                next_node = pointers[split]
            else:
                next_node = 0
            block.write(b'>II', next_node, split)

            for n in range(prev_split + 1, split):
                if internal:
                    block.write(b'>I', pointers[n])
                entries[n].write(block)

            block.zero_fill()
            
            prev_split = split

        return (entries[best_split[0]], entries[best_split[1]])

    # Extract all of the entries from the specified list of `blocks',
    # separating them by the specified `pivots'.  Also computes the
    # amount of space used before each entry.
    def _extract(self, blocks, pivots):
        pointers = []
        entries = []
        before = []
        total = 0
        ppivots = pivots + [None]
        for b,p in zip(blocks, ppivots):
            b.seek(0)
            next_node, count = b.read(b'>II')
            for n in range(count):
                pos = b.tell()
                if next_node:
                    ptr = b.read(b'>I')[0]
                    pointers.append(ptr)
                e = DSStoreEntry.read(b)
                entries.append(e)
                before.append(total)
                total += b.tell() - pos
            if next_node:
                pointers.append(next_node)
            if p:
                entries.append(p)
                before.append(total)
                total += p.byte_length()
                if next_node:
                    total += 4
        before.append(total)

        return (entries, pointers, before)

    # Rebalance the specified `node', whose path from the root is `path'.
    def _rebalance(self, path, node):
        # Can't rebalance the root
        if not path:
            return

        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            
            with self._get_block(path[-1]) as parent:
                # Find the left and right siblings and respective pivots
                parent_next, parent_count = parent.read(b'>II')
                left_pos = None
                left_node = None
                left_pivot = None
                node_pos = None
                right_pos = None
                right_node = None
                right_pivot = None
                prev_e = prev_ptr = prev_pos = None
                for n in range(parent_count):
                    pos = parent.tell()
                    ptr = parent.read(b'>I')[0]
                    e = DSStoreEntry.read(parent)

                    if ptr == node:
                        node_pos = pos
                        right_pivot = e
                        left_pos = prev_pos
                        left_pivot = prev_e
                        left_node = prev_ptr
                    elif prev_ptr == node:
                        right_node = ptr
                        right_pos = pos
                        break
                
                    prev_e = e
                    prev_ptr = ptr
                    prev_pos = pos

                if parent_next == node:
                    node_pos = parent.tell()
                    left_pos = prev_pos
                    left_pivot = prev_e
                    left_node = prev_ptr
                elif right_node is None:
                    right_node = parent_next
                    right_pos = parent.tell()

                parent_used = parent.tell()
            
            if left_node and right_node:
                with self._get_block(left_node) as left, \
                    self._get_block(right_node) as right:
                    blocks = [left, block, right]
                    pivots = [left_pivot, right_pivot]
                    
                    entries, pointers, before = self._extract(blocks, pivots)

                    # If there's a chance that we could use two pages instead
                    # of three, go for it
                    pivots = self._split2(blocks, entries, pointers,
                                           before, bool(next_node))
                    if pivots is None:
                        ptrs = [left_node, node, right_node]
                        pivots = self._split3(blocks, entries, pointers,
                                               before, bool(next_node))
                    else:
                        if pivots:
                            ptrs = [left_node, node]
                        else:
                            ptrs = [left_node]
                            self._store.release(node)
                            self._nodes -= 1
                            node = left_node
                        self._store.release(right_node)
                        self._nodes -= 1
                        self._dirty = True
                        
                    # Remove the pivots from the parent
                    with self._get_block(path[-1]) as parent:
                        if right_node == parent_next:
                            parent.seek(left_pos)
                            parent.delete(right_pos - left_pos)
                            parent_next = left_node
                        else:
                            parent.seek(left_pos + 4)
                            parent.delete(right_pos - left_pos)
                        parent.seek(0)
                        parent_count -= 2
                        parent.write(b'>II', parent_next, parent_count)
                        self._records -= 2
                        
                    # Replace with those in pivots
                    for e,rp in zip(pivots, ptrs[1:]):
                        self._insert_inner(path[:-1], path[-1], e, rp)
            elif left_node:
                with self._get_block(left_node) as left:
                    blocks = [left, block]
                    pivots = [left_pivot]

                    entries, pointers, before = self._extract(blocks, pivots)

                    pivots = self._split2(blocks, entries, pointers,
                                           before, bool(next_node))

                    # Remove the pivot from the parent
                    with self._get_block(path[-1]) as parent:
                        if node == parent_next:
                            parent.seek(left_pos)
                            parent.delete(node_pos - left_pos)
                            parent_next = left_node
                        else:
                            parent.seek(left_pos + 4)
                            parent.delete(node_pos - left_pos)
                        parent.seek(0)
                        parent_count -= 1
                        parent.write(b'>II', parent_next, parent_count)
                        self._records -= 1

                    # Replace the pivot
                    if pivots:
                        self._insert_inner(path[:-1], path[-1], pivots[0], node)
            elif right_node:
                with self._get_block(right_node) as right:
                    blocks = [block, right]
                    pivots = [right_pivot]

                    entries, pointers, before = self._extract(blocks, pivots)

                    pivots = self._split2(blocks, entries, pointers,
                                           before, bool(next_node))

                    # Remove the pivot from the parent
                    with self._get_block(path[-1]) as parent:
                        if right_node == parent_next:
                            parent.seek(pos)
                            parent.delete(right_pos - node_pos)
                            parent_next = node
                        else:
                            parent.seek(pos + 4)
                            parent.delete(right_pos - node_pos)
                        parent.seek(0)
                        parent_count -= 1
                        parent.write(b'>II', parent_next, parent_count)
                        self._records -= 1

                    # Replace the pivot
                    if pivots:
                        self._insert_inner(path[:-1], path[-1], pivots[0],
                                           right_node)

        if not path and not parent_count:
            self._store.release(path[-1])
            self._nodes -= 1
            self._dirty = True
            self._rootnode = node
        else:
            count, used = self._block_usage(path[-1])
    
            if used < self._page_size // 2:
                self._rebalance(path[:-1], path[-1])

    # Delete from the leaf node `node'.  `filename_lc' has already been
    # lower-cased.
    def _delete_leaf(self, node, filename_lc, code):
        found = False
        
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')

            for n in range(count):
                pos = block.tell()
                e = DSStoreEntry.read(block)
                if e.filename.lower() == filename_lc \
                  and (code is None or e.code == code):
                    block.seek(pos)
                    block.delete(e.byte_length())
                    found = True
                    
                    # This does not affect the loop; THIS IS NOT A BUG
                    count -= 1

                    self._records -= 1
                    self._dirty = True
                    
            if found:
                used = block.tell()
                
                block.seek(0)
                block.write(b'>II', next_node, count)
            
                return used < self._page_size // 2
            else:
                return False

    # Remove the largest entry from the subtree starting at `node' (with
    # path from root `path').  Returns a tuple (rebalance, entry) where
    # rebalance is either None if no rebalancing is required, or a
    # (path, node) tuple giving the details of the node to rebalance.
    def _take_largest(self, path, node):
        path = list(path)
        rebalance = None
        while True:
            with self._get_block(node) as block:
                next_node, count = block.read(b'>II')

                if next_node:
                    path.append(node)
                    node = next_node
                    continue

                for n in range(count):
                    pos = block.tell()
                    e = DSStoreEntry.read(block)

                count -= 1
                block.seek(0)
                block.write(b'>II', next_node, count)

                if pos < self._page_size // 2:
                    rebalance = (path, node)
                break

        return rebalance, e

    # Delete an entry from an inner node, `node'
    def _delete_inner(self, path, node, filename_lc, code):
        rebalance = False
        
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')

            for n in range(count):
                pos = block.tell()
                ptr = block.read(b'>I')[0]
                e = DSStoreEntry.read(block)
                if e.filename.lower() == filename_lc \
                  and (code is None or e.code == code):
                    # Take the largest from the left subtree
                    rebalance, largest = self._take_largest(path, ptr)

                    # Delete this entry
                    if n == count - 1:
                        right_ptr = next_node
                        next_node = ptr
                        block.seek(pos)
                    else:
                        right_ptr = block.read(b'>I')[0]
                        block.seek(pos + 4)
                        
                    block.delete(e.byte_length() + 4)

                    count -= 1
                    block.seek(0)
                    block.write(b'>II', next_node, count)

                    self._records -= 1
                    self._dirty = True
                    
                    break
                    
        # Replace the pivot value
        self._insert_inner(path, node, largest, right_ptr)

        # Rebalance from the node we stole from
        if rebalance:
            self._rebalance(rebalance[0], rebalance[1])
            return True
        return False

    def delete(self, filename, code):
        """Delete an item, identified by ``filename`` and ``code``
        from the B-Tree."""
        if isinstance(filename, DSStoreEntry):
            code = filename.code
            filename = filename.filename

        # If we're deleting *every* node for "filename", we must recurse
        if code is None:
            ###TODO: Fix this so we can do bulk deletes
            raise ValueError('You must delete items individually.  Sorry')

        # Otherwise, we're deleting *one* specific node
        filename_lc = filename.lower()
        path = []
        node = self._rootnode
        while True:
            with self._get_block(node) as block:
                next_node, count = block.read(b'>II')
                if next_node:
                    for n in range(count):
                        ptr = block.read(b'>I')[0]
                        e = DSStoreEntry.read(block)
                        e_lc = e.filename.lower()
                        if filename_lc < e_lc \
                            or (filename_lc == e_lc and code < e.code):
                            next_node = ptr
                            break
                        elif filename_lc == e_lc and code == e.code:
                            self._delete_inner(path, node, filename_lc, code)
                            return
                    path.append(node)
                    node = next_node
                else:
                    if self._delete_leaf(node, filename_lc, code):
                        self._rebalance(path, node)
                    return

    # Find implementation
    def _find(self, node, filename_lc, code=None):
        with self._get_block(node) as block:
            next_node, count = block.read(b'>II')
            if next_node:
                for n in range(count):
                    ptr = block.read(b'>I')[0]
                    e = DSStoreEntry.read(block)
                    if filename_lc < e.filename.lower():
                        for e in self._find(ptr, filename_lc, code):
                            yield e
                        return
                    elif filename_lc == e.filename.lower():
                        if code is None or (code and code < e.code):
                            for e in self._find(ptr, filename_lc, code):
                                yield e
                        if code is None or code == e.code:
                            yield e
                        elif code < e.code:
                            return
                for e in self._find(next_node, filename_lc, code):
                    yield e
            else:
                for n in range(count):
                    e = DSStoreEntry.read(block)
                    if filename_lc == e.filename.lower():
                        if code is None or code == e.code:
                            yield e
                        elif code < e.code:
                            return
                        
    def find(self, filename, code=None):
        """Returns a generator that will iterate over matching entries in
        the B-Tree."""
        if isinstance(filename, DSStoreEntry):
            code = filename.code
            filename = filename.filename

        filename_lc = filename.lower()
        
        return self._find(self._rootnode, filename_lc, code)

    def __len__(self):
        return self._records

    def __iter__(self):
        return self._traverse(self._rootnode)

    class Partial(object):
        """This is used to implement indexing."""
        def __init__(self, store, filename):
            self._store = store
            self._filename = filename

        def __getitem__(self, code):
            if code is None:
                raise KeyError('no such key - [%s][None]' % self._filename)
            
            try:
                item = next(self._store.find(self._filename, code))
            except StopIteration:
                raise KeyError('no such key - [%s][%s]' % (self._filename,
                               code))

            if not isinstance(item.type, (str, unicode)):
                return item.value
            
            return (item.type, item.value)
            
        def __setitem__(self, code, value):
            if code is None:
                raise KeyError('bad key - [%s][None]' % self._filename)

            codec = codecs.get(code, None)
            if codec:
                entry_type = codec
                entry_value = value
            else:
                entry_type = value[0]
                entry_value = value[1]
            
            self._store.insert(DSStoreEntry(self._filename, code,
                                             entry_type, entry_value))

        def __delitem__(self, code):
            if code is None:
                raise KeyError('no such key - [%s][None]' % self._filename)

            self._store.delete(self._filename, code)

        def __iter__(self):
            for item in self._store.find(self._filename):
                yield item

    def __getitem__(self, filename):
        return self.Partial(self, filename)
    
