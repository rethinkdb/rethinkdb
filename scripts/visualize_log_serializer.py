# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
from collections import namedtuple
from parse_binary import *
import sys, os, traceback



def escape(string):
        return string.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def print_anchor(obj):
    print """<a name="obj-%d"/>""" % id(obj)

parse_block_id = parse_uint64_t



# This is sort of unintuitive, so pay attention.
#
# The visualizer needs to be robust even if there is corruption in a file. One approach to this is
# to carefully check every part of the file as we parse it; if there is an error, we can then record
# it and then move on. However, this approach breaks down if we forget to check something; the
# visualizer crashes without producing any output, and the traceback is all that the programmer has
# to figure out what went wrong in the log-serializer.
#
# The approach that we use instead is to trap exceptions. Whenever we parse a chunk of the file, we
# use the try_parse() function. If an exception occurs during the parsing, it returns a BadChunk
# object that holds the error message; if not, it returns a GoodChunk object that returns the
# original object. This way, if something goes wrong, we get an error report embedded in the file
# instead of on the console, even if we didn't plan to check for that error condition.
#
# try_store() is the companion to try_parse(). It is responsible for storing the [Good|Bad]Chunk
# that was produced by try_parse(). It uses a user-defined function to record the chunk. If the
# user-defined function fails, then we assume that it wasn't valid to try to parse that chunk of
# memory (perhaps it overlapped with another existing chunk) and we produce a BadChunkRef object
# instead
#
# The difference between BadChunk and BadChunkRef is that you get a BadChunk if you were looking
# in the right place for the data, but it was corrupted, and you get a BadChunkRef if your pointer
# is bad.

class Chunk(object):
    pass

class GoodChunk(Chunk):
    
    def __init__(self, offset, length, name, obj):
        self.offset = offset
        self.length = length
        self.name = name
        self.chunk_obj = obj
        self.chunk_ok = True
    
    def ref_as_html(self):
        return """<a href="#obj-%d">0x%x</a>""" % (id(self), self.offset)
    
    def chunk_print_html(self):
        print """<div class="block">"""
        print """<a name="obj-%d"/>""" % id(self)
        print """<h1>0x%x - 0x%x: %s</h1>""" % (self.offset, self.offset + self.length - 1, self.name)
        self.chunk_obj.print_html()
        print """</div>"""

class BadChunk(Chunk):
    
    def __init__(self, offset, length, name, contents, msg):
        self.offset = offset
        self.length = length
        self.name = name
        self.msg = msg
        self.contents = contents
        self.chunk_ok = False
    
    def ref_as_html(self):
        return """<a href="#obj-%d">0x%x</a>""" % (id(self), self.offset)
    
    def chunk_print_html(self):
        print """<div class="block">"""
        print """<a name="obj-%d"/>""" % id(self)
        print """<h1>0x%x - 0x%x: %s</h1>""" % (self.offset, self.offset + self.length - 1, self.name)
        print """<pre style="color: red">%s</pre>""" % escape(self.msg)
        print """<div class="hexdump">%s</div>""" % \
            " ".join(x.encode("hex") for x in self.contents)
        print """</div>"""

class BadChunkRef(Chunk):
    
    def __init__(self, offset, length, name, msg):
        self.offset = offset
        self.length = length
        self.name = name
        self.msg = msg
        self.chunk_ok = False
    
    def ref_as_html(self):
        return """<span style="color: red">Bad reference to 0x%x: <code>%s</code></span>""" % \
            (self.offset, escape(self.msg))
    
    def chunk_print_html(self):
        print """<span style="color: red">Bad reference (but we're so screwed that we think it's
inside of a block): %s</span>""" % self.msg

def try_parse(db, offset, length, name, cls, *args):
    
    if offset < 0:
        return BadChunkRef(offset, length, name, "Negative offset")
    elif length <= 0:
        return BadChunkRef(offset, length, name, "Negative or zero length")
    elif offset + length > len(db.block):
        return BadChunkRef(offset, length, name,
            "Chunk 0x%x - 0x%x is beyond end of file (0x%x)" % \
            (offset, offset + length - 1, len(db.block)))
    
    try:
        x = cls.from_data(db, offset, *args)
        assert isinstance(x, cls)
    except Exception:
        chunk = BadChunk(offset, length, name, db.block[offset : offset + length], traceback.format_exc())
    else:
        chunk = GoodChunk(offset, length, name, x)
    
    return chunk

def try_store(chunk, fun, *args):
    
    if isinstance(chunk, BadChunkRef):
        return chunk
    
    try:
        fun(chunk, *args)
    except Exception:
        chunk = BadChunkRef(chunk.offset, chunk.length, chunk.name, traceback.format_exc())
    
    return chunk



class Database(object):
    
    def __init__(self, block):
        
        self.block = block
        self.extents = {}
        
        # Determine configuration info
        
        self.device_block_size = 0x1000
        initial_static_header = StaticHeader.from_data(self, 0)
        self.mb_extents = [0, 4]
        self.extent_size = initial_static_header.sh.extent_size
        self.block_size = initial_static_header.sh.btree_block_size
        
        assert len(self.block) % self.extent_size == 0
        assert self.extent_size % self.block_size == 0
        assert self.block_size % self.device_block_size == 0
        
        # Read metablock extents
        
        metablock_versions = {}
        metablock_extents = []
        for mb_extent in self.mb_extents:
            offset = mb_extent * self.extent_size
            if offset >= len(self.block): continue
            
            extent = try_store(try_parse(self, offset, self.extent_size, "Metablock Extent", MetablockExtent), self.add_extent)
            metablock_extents.append(extent)
            
            if extent.chunk_ok:
                for mb in extent.chunk_obj.metablocks:
                    if mb.chunk_ok:
                        metablock_versions[mb.chunk_obj.mb.version] = mb
        
        # Choose a metablock
        
        if metablock_versions:
            self.use_metablock(metablock_versions[max(metablock_versions.keys())])
            
            # Notify the metablock extents that we found a valid metablock so that they don't print
            # all of the invalid ones
            for extent in metablock_extents:
                if extent.chunk_ok:
                    extent.chunk_obj.found_a_valid_metablock = True
        else:
            self.metablock = None
        
        # Fill in empty extents with placeholders
        
        for offset in xrange(0, max(self.extents.keys())+1, self.extent_size):
            if offset not in self.extents:
                self.add_extent(GoodChunk(offset, self.extent_size, "Unused Extent", UnusedExtent()))
    
    def add_extent(self, extent):
        
        assert isinstance(extent, Chunk)
        assert extent.name.endswith("Extent")
        
        if extent.offset % self.extent_size != 0:
            raise ValueError("Misaligned extent: 0x%x is not a multiple of 0x%x." % \
                (extent.offset, self.extent_size))
        
        if extent.offset in self.extents:
            raise ValueError("Duplicate extent: 0x%x was parsed already." % extent.offset)
        
        self.extents[extent.offset] = extent
    
    def use_metablock(self, mb):
        
        self.metablock = mb
        lba_index_part = mb.chunk_obj.mb.metablock.lba_index_part
        
        # Read the current LBA extent
    
        if lba_index_part.last_lba_extent_offset >= 0:
            first_lba_extent = try_parse(self, lba_index_part.last_lba_extent_offset, self.extent_size, "LBA Extent", LBAExtent,
                lba_index_part.last_lba_extent_entries_count)
            first_lba_extent = try_store(first_lba_extent, self.add_extent)
        else:
            first_lba_extent = None
        
        # Read the LBA superblock and its sub-extents
        
        if lba_index_part.lba_superblock_offset >= 0:
            
            size = lba_index_part.lba_superblock_entries_count * 16 + 1
            while size % self.device_block_size != 0: size += 1
            
            lba_superblock = try_parse(self, lba_index_part.lba_superblock_offset, size, "LBA Superblock", LBASuperblock,
                lba_index_part.lba_superblock_entries_count)
            lba_superblock = try_store(lba_superblock, LBASuperblockExtent.store_superblock, self)
            
        else:
            lba_superblock = None
        
        # Reconstruct the LBA index
        
        lba_extents = []
        if lba_superblock and lba_superblock.chunk_ok:
            lba_extents.extend(lba_superblock.chunk_obj.lba_extents)
        if first_lba_extent:
            lba_extents.append(first_lba_extent)
        
        lba = {}
        data_blocks = {}
        
        for extent in reversed(lba_extents):
            if extent.chunk_ok:
                for pair in reversed(extent.chunk_obj.pairs):
                    if isinstance(pair, LBAPair) and pair.block_id not in lba:
                        lba[pair.block_id] = pair.block_offset
                        data_blocks[pair.block_id] = self.use_lba_pair(pair)
        
        self.metablock.chunk_obj.was_used(first_lba_extent, lba_superblock, lba, data_blocks)
    
    def use_lba_pair(self, pair):
        
        if pair.block_offset == "delete":
            data_block = None
        else:
            data_block = try_parse(self, pair.block_offset, self.block_size, "Data Block %d" % pair.block_id, DataBlock)
            data_block = try_store(data_block, DataBlockExtent.store_data_block, self)
        
        pair.was_used(data_block)
        return data_block
    
    def print_html(self):
        
        print """<h1>Database</h1>"""
        
        print """<p>End of file is at 0x%x</p>""" % len(self.block)
        
        if self.metablock:
            print """<p>Most recent metablock: %s</p>""" % self.metablock.ref_as_html()
        else:
            print """<p>No valid metablocks found.</p>"""
        
        for i in sorted(self.extents.keys()):
            self.extents[i].chunk_print_html()



class UnusedExtent(object):
    def print_html(self):
        print """<p>Nothing in this extent is reachable from the most recent metablock.</p>"""



class MetablockExtent(object):
    
    @classmethod
    def from_data(cls, db, offset):
        
        static_header = try_parse(db, offset, db.device_block_size, "Static Header", StaticHeader)
        
        metablocks = []
        for o in xrange(db.device_block_size, db.extent_size, db.device_block_size):
            metablocks.append(try_parse(db, offset + o, db.device_block_size, "Metablock", Metablock))
        
        return MetablockExtent(static_header, metablocks)
    
    def __init__(self, static_header, metablocks):
        
        self.static_header = static_header
        self.metablocks = metablocks
        
        # If there are no valid metablocks in the entire database, we want to print each metablock
        # that we tried and explain why it failed. If there is a valid metablock, that information
        # is just junk. The database sets found_a_valid_metablock to True if it found at least one
        # valid metablock.
        self.found_a_valid_metablock = False
    
    def print_html(self):
        
        self.static_header.chunk_print_html()
        
        for metablock in self.metablocks:
            if not self.found_a_valid_metablock or metablock.chunk_ok:
                metablock.chunk_print_html()

class StaticHeader(object):
    
    @classmethod
    def from_data(cls, db, offset):
        
        static_header, parse_static_header = make_struct("static_header", [
            (None, parse_constant("RethinkDB\0")),
            (None, parse_constant("0.0.0\0")),
            (None, parse_constant("BTree Blocksize:\0")),
            ("btree_block_size", parse_uint64_t),
            (None, parse_constant("Extent Size:\0")),
            ("extent_size", parse_uint64_t),
            ])
        
        sh = parse_static_header(db.block, offset)[0]
        return StaticHeader(sh)
    
    def __init__(self, sh):
        self.sh = sh
    
    def print_html(self):
        print """<p>Block size: %d</p>""" % self.sh.btree_block_size
        print """<p>Extent size: %d</p>""" % self.sh.extent_size

class Metablock(object):
    
    # The parser object is a little bit slow to construct, and many many metablocks are read, so
    # we cache the parser object.
    
    crc_metablock_parser_cache = {}
    @classmethod
    def make_parser(c, markers):
        
        if markers not in c.crc_metablock_parser_cache:
        
            def maybe(p):
                if markers: return p
                else: return parse_padding(0)
            
            extent_manager_mb, parse_extent_manager_mb = make_struct("extent_manager_mb", [
                ("last_extent", parse_off64_t),
                ])
            
            lba_index_mb, parse_lba_index_mb = make_struct("lba_index_mb", [
                ("last_lba_extent_offset", parse_off64_t),
                ("last_lba_extent_entries_count", parse_int),
                (None, parse_padding(4)),
                ("lba_superblock_offset", parse_off64_t),
                ("lba_superblock_entries_count", parse_int),
                (None, parse_padding(4)),
                ])
            
            data_block_mb, parse_data_block_mb = make_struct("data_block_mb", [
                ("last_data_extent", parse_off64_t),
                ("blocks_in_last_data_extent", parse_int),
                (None, parse_padding(4)),
                ])
            
            metablock_t, parse_metablock = make_struct("metablock_t", [
                ("extent_manager_part", parse_extent_manager_mb),
                ("lba_index_part", parse_lba_index_mb),
                ("data_block_manager_part", parse_data_block_mb),
                ])
            
            crc_metablock_t, parse_crc_metablock = make_struct("crc_metablock_t", [
                (None, maybe(parse_constant("metablock\xbd"))),
                (None, maybe(parse_constant("crc:\xbd"))),
                ("crc", parse_uint32_t),
                (None, maybe(parse_padding(1))),
                (None, maybe(parse_constant("version:"))),
                ("version", parse_int),
                ("metablock", parse_metablock),
                ])
            
            c.crc_metablock_parser_cache[markers] = parse_crc_metablock
        
        return c.crc_metablock_parser_cache[markers]
    
    @classmethod
    def from_data(c, db, offset):
        
        # Try to read the metablock with and without markers; if either works, we're good.
        
        try:
            mb = c.make_parser(True)(db.block, offset)[0]
            assert mb.version > 0
        except Exception, e:
            error1 = traceback.format_exc()
            try:
                mb = c.make_parser(False)(db.block, offset)[0]
                assert mb.version > 0
            except Exception, e:
                error2 = traceback.format_exc()
                raise ValueError("Invalid metablock.\n\n" + \
                    "Problem when trying to parse with markers:\n\n" + error1 + "\n" + \
                    "Problem when trying to parse without markers:\n\n" + error2)
        
        return Metablock(mb)
    
    def __init__(self, mb):
        self.mb = mb
        self.chosen = False
    
    def was_used(self, first_lba_extent, lba_superblock, lba, data_blocks):
        self.chosen = True
        self.first_lba_extent = first_lba_extent
        self.lba_superblock = lba_superblock
        self.lba = lba
        self.data_blocks = data_blocks
    
    def print_html(self):
        
        print """<table>"""
        
        print """<tr><td>CRC</td><td>0x%.8x</td></tr>""" % self.mb.crc
        print """<tr><td>Version</td><td>%d</td></tr>""" % self.mb.version
        
        print """<tr><td>Last extent</td><td>0x%x</td></tr>""" % \
            self.mb.metablock.extent_manager_part.last_extent
        
        print """<tr><td>Last LBA extent offset</td>"""
        if self.chosen and self.first_lba_extent:
            print """<td>%s</td>""" % self.first_lba_extent.ref_as_html()
        else:
            print """<td>0x%x</td>""" % self.mb.metablock.lba_index_part.last_lba_extent_offset
        print """</tr>"""
        
        print """<tr><td>Last LBA extent entries count</td><td>%d</td></tr>""" % \
            self.mb.metablock.lba_index_part.last_lba_extent_entries_count
        
        print """<tr><td>LBA superblock offset</td>"""
        if self.chosen and self.lba_superblock:
            print """<td>%s</td>""" % self.lba_superblock.ref_as_html()
        else:
            print """<td>0x%x</td>""" % self.mb.metablock.lba_index_part.lba_superblock_offset
        print """</tr>"""
        
        print """<tr><td>LBA superblock entries count</td><td>%d</td></tr>""" % \
            self.mb.metablock.lba_index_part.lba_superblock_entries_count
        
        print """<tr><td>Last data extent</td><td>0x%x</td></tr>""" % \
            self.mb.metablock.data_block_manager_part.last_data_extent
        print """<tr><td>Blocks in last data extent</td><td>%d</td></tr>""" % \
            self.mb.metablock.data_block_manager_part.blocks_in_last_data_extent
        
        print """</table>"""



class LBASuperblockExtent(object):
    
    @classmethod
    def store_superblock(self, chunk, db):
        
        offset = chunk.offset - chunk.offset % db.extent_size
        if offset not in db.extents:
            c = GoodChunk(offset, db.extent_size, "LBA Superblock Extent", LBASuperblockExtent())
            db.add_extent(c)
        
        if db.extents[offset].name != "LBA Superblock Extent":
            raise ValueError("Extent at 0x%x is a %r." % (offset, db.extents[offset].name))
        assert db.extents[offset].chunk_ok
        assert isinstance(db.extents[offset].chunk_obj, LBASuperblockExtent)
        
        db.extents[offset].chunk_obj.children.add(chunk)
    
    def __init__(self):
        
        self.children = set()
    
    def print_html(self):
        
        for child in sorted(self.children, lambda x,y: cmp(x.offset, y.offset)):
            child.chunk_print_html()

class LBASuperblock(object):
    
    @classmethod
    def from_data(cls, db, offset, how_many_lba_extents):
        
        lba_extents = []
        start_offset = offset
        
        _, offset = parse_constant("lbasuper")(db.block, offset)
        while offset % 16 != 0: offset += 1
        
        for i in xrange(how_many_lba_extents):
        
            lba_extent_offset, offset = parse_off64_t(db.block, offset)
            how_many_pairs, offset = parse_int(db.block, offset)
            offset += 4   # Padding after the int
            
            lba_extent = try_parse(db, lba_extent_offset, db.extent_size, "LBA Extent", LBAExtent, how_many_pairs)
            lba_extent = try_store(lba_extent, db.add_extent)
            
            lba_extents.append(lba_extent)
        
        while offset % db.device_block_size != 0:
            offset += 1
        
        return LBASuperblock(lba_extents)
    
    def __init__(self, lba_extents):
        self.lba_extents = lba_extents
    
    def print_html(self):
        for extent in self.lba_extents:
            print """<p>Extent: %s</p>""" % extent.ref_as_html()

class LBAExtent(object):
    
    @classmethod
    def from_data(cls, db, offset, count):
        
        _, offset = parse_constant("lbamagic")(db.block, offset);
        while offset % 16 != 0: offset += 1
        
        pairs = []
        
        for i in xrange(count):
            
            block_id, offset = parse_block_id(db.block, offset)
            block_offset, offset = parse_off64_t(db.block, offset)
            
            if block_id == 0xFFFFFFFFFFFFFFFF:
                assert block_offset == -1
                pairs.append(LBAPaddingPair())
            
            else:
                if block_offset == -1:
                    block_offset = "delete"
                pairs.append(LBAPair(block_id, block_offset))
        
        return LBAExtent(pairs)
    
    def __init__(self, pairs):
        self.pairs = pairs
    
    def print_html(self):
        print """<div style="-webkit-column-width: 310px">"""
        print """<table>"""
        print """<tr><th>Block ID</th><th>Offset</th></tr>"""
        for entry in self.pairs:
            entry.print_html()
        print """</table>"""
        print """</div>"""

class LBAPaddingPair(object):
    
    def print_html(self):
        print """<tr><td colspan=2><i>(padding)</i></td></tr>"""

class LBAPair(object):
    
    def __init__(self, block_id, block_offset):
        
        self.block_id = block_id
        self.block_offset = block_offset
        self.chosen = False
    
    def was_used(self, data_block):
        self.chosen = True
        self.data_block = data_block
    
    def print_html(self):
        print """<tr>"""
        print """<td>%d</td>""" % self.block_id
        if self.chosen:
            if self.block_offset == "delete":
                print """<td>delete</td>"""
            else:
                print """<td>%s</td>""" % self.data_block.ref_as_html()
        else:
            if self.block_offset == "delete":
                print """<td><i>delete</i></td>"""
            else:
                print """<td><i>0x%x</i></td>""" % self.block_offset
        print """</tr>"""



class DataBlockExtent(object):
    
    @classmethod
    def store_data_block(self, chunk, db):
        
        offset = chunk.offset - chunk.offset % db.extent_size
        
        if offset not in db.extents:
            ext = GoodChunk(offset, db.extent_size, "Data Block Extent", DataBlockExtent())
            db.add_extent(ext)
        
        if db.extents[offset].name != "Data Block Extent":
            raise ValueError("Extent at 0x%x is a %r." % (offset, db.extents[offset].name))
        assert db.extents[offset].chunk_ok
        assert isinstance(db.extents[offset].chunk_obj, DataBlockExtent)
        
        db.extents[offset].chunk_obj.children.add(chunk)
    
    def __init__(self):
        self.children = set()
    
    def print_html(self):
        for child in sorted(self.children, lambda x,y: cmp(x.offset, y.offset)):
            child.chunk_print_html()

class DataBlock(object):
    
    @classmethod
    def from_data(cls, db, offset):
        contents = db.block[offset : offset + db.block_size]
        return DataBlock(contents)
    
    def __init__(self, contents):
        self.contents = contents
    
    def print_html(self):
        print """<div class="hexdump">%s</div>""" % \
            " ".join(x.encode("hex") for x in self.contents)



def file_to_database(filename):
    
    with file(filename) as f:
        text = f.read()
    
    return Database(text)

def database_to_html(db, filename):
    
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
.hexdump {
    color: green;
    font-family: monospace;
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

            db.print_html()
            
            print """
    </body>
</html>
            """
            
        finally:
            sys.stdout = sys.__stdout__

def database_to_blocks(db):
    
    if not db.metablock or not db.metablock.chunk_ok:
        return {}
    
    else:
        blocks = {}
        for (block_id, data_block) in db.metablock.chunk_obj.data_blocks.iteritems():
            if data_block is not None:
                if data_block.chunk_ok:
                    blocks[block_id] = data_block.chunk_obj.contents
        return blocks

if __name__ == "__main__":
    
    if len(sys.argv) == 3:
        database_to_html(file_to_database(sys.argv[1]), sys.argv[2])
    else:
        print "Usage: %s data_file output.html" % sys.argv[0]
