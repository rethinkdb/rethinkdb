#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
from collections import namedtuple
from parse_binary import *
import sys, traceback

def escape(string):
    return string.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

class Block(object):
    pass

class GoodBlock(Block):
    def __init__(self, id, obj):
        self.id = id
        self.block_obj = obj
        self.block_ok = True
    def ref_as_html(self):
        return """<a href="#buf-%d">Block %d</a>""" % (id(self), self.id)
    def block_print_html(self):
        print """<div class="block">"""
        print """<a name="buf-%d"/>""" % id(self)
        print """<h1>Block %d: %s</h1>""" % (self.id, self.block_obj.name)
        self.block_obj.print_html()
        print """</div>"""

class BadBlock(Block):
    def __init__(self, id, name, msg):
        self.id = id
        self.name = name
        self.msg = msg
        self.block_ok = False
    def ref_as_html(self):
        return """<a href="#buf-%d">Block %d</a>""" % (id(self), self.id)
    def block_print_html(self):
        print """<div class="block">"""
        print """<a name="buf-%d"/>""" % id(self)
        print """<h1>Block %d: %s</h1>""" % (self.id, self.name)
        print """<pre style="color: red">%s</pre>""" % escape(self.msg)
        print """</div>"""

class BadBlockRef(Block):
    def __init__(self, id, msg):
        self.id = id
        self.msg = msg
    def ref_as_html(self):
        return """<span style="color:red">Bad reference to block %d: <code>%s</code></span>""" % \
            (self.id, escape(self.msg))
    def block_print_html(self):
        print """<span style="color:red">This shouldn't go here! But here it is anyway: %s</span>""" % \
            escape(self.msg)

class BlockGroup(object):
    def __init__(self, blocks):
        self.blocks = blocks
        self.values = {}
    def try_parse(self, block_id, cls):
        if block_id not in self.blocks:
            return BadBlockRef(block_id, "Not a real block.")
        elif block_id in self.values:
            return BadBlockRef(block_id, "Already used.")
        else:
            try:
                value = cls.from_block(self, self.blocks[block_id])
                assert isinstance(value, cls)
            except Exception, e:
                b = BadBlock(block_id, cls.name, traceback.format_exc())
            else:
                b = GoodBlock(block_id, value)
            self.values[block_id] = b
            return b

class UnusedBlock(object):
    name = "Unused Block"
    def print_html(self):
        print """<p>This block is not reachable from the root.</p>"""

def blocks_to_btree(blocks):
    
    assert isinstance(blocks, dict)
    
    bgrp = BlockGroup(blocks)
    superblock = bgrp.try_parse(0, Superblock)
    
    for block_id in bgrp.blocks:
        if block_id not in bgrp.values:
            bgrp.values[block_id] = GoodBlock(block_id, UnusedBlock())
    
    return (superblock, bgrp.values)



parse_block_id = parse_uint64_t

class BtreeKey(object):
    
    @classmethod
    def parse(cls, block, offset = 0):
        
        size, offset = parse_uint8_t(block, offset)
        value, offset = block[offset : offset + size], offset + size
        
        return cls(value), offset
    
    def __init__(self, name):
        self.name = name
    
    def print_html(self):
        if len(self.name) < 15:
            print """<code>%s</code>""" % escape(self.name.encode("string-escape"))
        else:
            print """<code>%s</code>...<code>%s</code>""" % \
                (escape(self.name[:8].encode("string-escape")),
                 escape(self.name[-8:].encode("string-escape")))

class BtreeValue(object):
    
    @classmethod
    def parse(cls, bgrp, block, offset = 0):
        
        size, offset = parse_uint8_t(block, offset)
        md_flags, offset = parse_uint8_t(block, offset)
        
        value_start_offset = offset
        
        if md_flags & 0x01: flags, offset = parse_uint32_t(block, offset)
        else: flags = None
        
        if md_flags & 0x02: cas, offset = parse_uint64_t(block, offset)
        else: cas = None
        
        if md_flags & 0x04: exptime, offset = parse_uint32_t(block, offset)
        else: exptime = None
        
        if md_flags & 0x08:
            # Large value
            assert size == 8
            large_value_size, offset = parse_uint32_t(block, offset)
            superblock_id, offset = parse_block_id(block, offset)
            superblock = bgrp.try_parse(superblock_id, BtreeLargeValueSuperblock, large_value_size)
            return BtreeLargeValue(large_value_size, superblock, flags, cas, exptime)
            
        else:
            # Small value
            value, offset = block[offset : value_start_offset + size], value_start_offset + size
            return BtreeSmallValue(value, cas, flags, exptime), offset
    
    def __init__(self, flags, cas, exptime):
        self.flags = flags
        self.cas = cas
        self.exptime = exptime

class BtreeSmallValue(BtreeValue):
    
    def __init__(self, contents, flags, cas, exptime):
        BtreeValue.__init__(self, flags, cas, exptime)
        self.contents = contents
    
    def print_html(self):
        # TODO: CAS, flags, and exptime.
        if len(self.contents) < 15:
            print """<code>%s</code>""" % escape(self.contents.encode("string-escape"))
        else:
            print """<code>%s</code>...<code>%s</code>""" % \
                (escape(self.contents[:8].encode("string-escape")),
                 escape(self.contents[-8:].encode("string-escape")))

class BtreeLargeValue(BtreeValue):
    
    def __init__(self, size, superblock, flags, cas, exptime):
        BtreeValue.__init__(self, flags, cas, exptime)
        self.size = size
        self.superblock = superblock
    
    def print_html(self):
        print """large value: %s""" % self.superblock.ref_as_html()

class BtreeLargeValueSuperblock(object):
    
    name = "Large Value Superblock"
    
    @classmethod
    def from_block(cls, bgrp, block, size):
        
        offset = 0
        
        size2, offset = parse_uint32_t(block, offset)
        if size2 != size:
            raise ValueError("Leaf node said this large block was %d bytes, but the index block " \
                "says it's %d bytes." % (size, size2))
        
        num_segments, offset = parse_uint16_t(block, offset)
        first_block_offset, offset = parse_uint16_t(block, offset)
        segment_ids, offset = parse_array(parse_block_id, num_segments)
        
        next_block_offset = first_block_offset
        size_left = size
        segments = []
        for segment_id in segment_ids:
            next_block_size = min(size_left, len(block) - next_block_offset)
            segments.append(bgrp.try_parse(segment_id, BtreeLargeValueBlock, next_block_offset, next_block_size))
            size_left -= next_block_size
            next_block_offset = 0
        assert size_left == 0
        
        return BtreeLargeValueSuperblock(size, first_block_offset, segments)
    
    def __init__(self, size, first_block_offset, segments):
        self.size = size
        self.first_block_offset = first_block_offset
        self.segments = segments
    
    def print_html(self):
        print """<p>Size: %d bytes</p>""" % self.size
        print """<p>Offset into first block: %d bytes</p>""" % self.first_block_offset
        for segment in self.segments:
            print """<p>Segment: %s</p>""" % segment.ref_as_html()

class BtreeLargeValueBlock(object):
    
    name = "Large Value Block"
    
    @classmethod
    def from_block(cls, bgrp, block, start = 0, length = 0):
        return BtreeLargeValueBlock(block[start : start + length])
    
    def __init__(self, contents):
        self.contents = contents
    
    def print_html(self):
        print """<code>%s</code>""" % escape(self.contents.encode("string-escape"))

class BtreeNode(object):
    
    name = "BTree Node"
    
    @classmethod
    def from_block(cls, bgrp, block):
        
        type = ord(block[0])
        if type == 1:
            return BtreeLeafNode.from_block(bgrp, block)
        elif type == 2:
            return BtreeInternalNode.from_block(bgrp, block)
        else:
            raise ValueError("First byte should be 1 or 2, got %d." % type)

class BtreeLeafNode(BtreeNode):
    
    name = "BTree Leaf Node"
    
    @classmethod
    def from_block(cls, bgrp, block):
        
        offset = 0
        
        type, offset = parse_int(block, offset)
        assert type == 1
        
        npairs, offset = parse_uint16_t(block, offset)
        frontmost_offset, offset = parse_uint16_t(block, offset)
        pair_offsets, offset = parse_array(parse_uint16_t, npairs)(block, offset)
        
        pairs = []
        for offset in pair_offsets:
            key, offset = BtreeKey.parse(block, offset)
            value, offset = BtreeValue.parse(bgrp, block, offset)
            pairs.append((key, value))
        
        return BtreeLeafNode(pairs)
    
    def __init__(self, pairs):
        self.pairs = pairs
    
    def print_html(self):
        
        print """<p>%d key/value pairs</p>""" % len(self.pairs)
        
        print """<div style="-webkit-column-width: 310px">"""
        print """<table>"""
        print """<tr><th>Key</th><th>Value</th></tr>"""
        for key, value in self.pairs:
            print """<tr><td>"""
            key.print_html()
            print """</td><td>"""
            value.print_html()
            print """</td></tr>"""
        print """</table>"""
        print """</div>"""
        
class BtreeInternalNode(BtreeNode):
    
    name = "BTree Internal Node"
    
    @classmethod
    def from_block(cls, bgrp, block):
        
        offset = 0
            
        type, offset = parse_int(block, offset)
        assert type == 2
        
        npairs, offset = parse_uint16_t(block, offset)
        frontmost_offset, offset = parse_uint16_t(block, offset)
        pair_offsets, offset = parse_array(parse_uint16_t, npairs)(block, offset)
        
        pairs = []
        for offset in pair_offsets:
            subtree_id, offset = parse_block_id(block, offset)
            subtree = bgrp.try_parse(subtree_id, BtreeNode)
            key, offset = BtreeKey.parse(block, offset)
            pairs.append((key, subtree))
        
        assert pairs[-1][0].name == ""   # Last pair is special
        pairs[-1] = (None, pairs[-1][1])
        
        return BtreeInternalNode(pairs)
    
    def __init__(self, pairs):
        self.pairs = pairs
    
    def print_html(self):
        
        print """<p>%d key/value pairs</p>""" % len(self.pairs)
        
        print """<table>"""
        print """<tr><th>Keys</th><th>Subtree</th></tr>"""
        for key, subtree in self.pairs:
            print """<tr>"""
            if key is None:
                print """<td>All other keys</td>"""
            else:
                print """<td>Keys up to """
                key.print_html()
                print """</td>"""
            print """<td>%s</td></tr>""" % subtree.ref_as_html()
        print """</table>"""

class Superblock(object):
    
    name = "Superblock"
    
    @classmethod
    def from_block(cls, bgrp, block):
        
        offset = 0
    
        btree_superblock_t, parse_superblock = make_struct("Superblock", [
            ("database_exists", parse_int),
            (None, parse_padding(4)),
            ("root_id", parse_block_id),
            ])
        sb = parse_superblock(block)[0]
        assert sb.database_exists == 1
        
        root = bgrp.try_parse(sb.root_id, BtreeNode)
        
        return Superblock(root)
    
    def __init__(self, root):
        self.root = root
    
    def print_html(self):
        
        print """<p>Root: %s</p>""" % self.root.ref_as_html()



def btree_to_html(btree, filename):
    
    superblock, values = btree
    
    with file(filename, "w") as f:
        sys.stdout = f
        try:
            print """
<html>
    <head>
        <style type="text/css">
h1 {
    font-size: 1.1em;
    font-weight: normal;
}
div.block {
    border: dashed 1px;
    margin: 0.2cm;
    padding: 0.2cm;
}
table {
    border-collapse: collapse;
}
td, th {
    border: gray solid 1px;
    width: 150px;
}
        </style>
    </head>
    <body>
            """
            
            print """<h1>BTree</h1>"""
            
            print """<p>Superblock: %s</p>""" % superblock.ref_as_html()
            
            for block_id in sorted(values.keys()):
                values[block_id].block_print_html()
            
            print """
    </body>
</html>
            """
        finally:
            sys.stdout = sys.__stdout__

if __name__ == "__main__":
    
    if len(sys.argv) == 3:
        from visualize_log_serializer import file_to_database, database_to_blocks
        btree_to_html(blocks_to_btree(database_to_blocks(file_to_database(sys.argv[1]))), sys.argv[2])
    
    else:
        print "Usage: %s data_file output.html" % sys.argv[0]
