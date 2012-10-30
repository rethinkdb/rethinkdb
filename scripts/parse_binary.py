# Copyright 2010-2012 RethinkDB, all rights reserved.
from collections import namedtuple
import struct

def parse_prim(format):
    def parse(block, offset = 0):
        value, = struct.unpack_from(format, block, offset)
        return (value, offset + struct.calcsize(format))
    return parse

parse_int = parse_prim('i')

parse_uint8_t = parse_prim('B')
parse_uint16_t = parse_prim('H')
parse_uint32_t = parse_prim('I')
parse_uint64_t = parse_prim('Q')

parse_off64_t = parse_prim('q')

def make_struct(name, names_and_parsers):
    ty = namedtuple(name, [x[0] for x in names_and_parsers if x[0] is not None])
    def parse(block, offset = 0):
        assert isinstance(block, str)
        assert isinstance(offset, (int, long))
        values = []
        for name, parser in names_and_parsers:
            value, offset = parser(block, offset)
            if name is not None: values.append(value)
        return ty(*values), offset
    return ty, parse

def parse_padding(size):
    def parse(block, offset = 0):
        return None, offset + size
    return parse

def parse_constant(string):
    def parse(block, offset = 0):
        val = block[offset : offset + len(string)]
        if val != string:
            raise ValueError("Expected %r, got %r." % (string, val))
        return None, offset + struct.calcsize("%ds" % len(string))
    return parse

def parse_array(parser, count):
    def parse(block, offset = 0):
        values = []
        for i in range(count):
            value, offset = parser(block, offset)
            values.append(value)
        return values, offset
    return parse

            