#!/usr/bin/python

# Copyright 2012. Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# This tests the SPLIT_BY_CHARACTERS rule.

import BoostBuild

def test_invalid(params, expected_error_line):
    t = BoostBuild.Tester(pass_toolset=0)
    t.write("file.jam", "SPLIT_BY_CHARACTERS %s ;" % params)
    t.run_build_system(["-ffile.jam"], status=1)
    t.expect_output_lines("[*] %s" % expected_error_line)
    t.cleanup()

def test_valid():
    t = BoostBuild.Tester(pass_toolset=0)
    t.write("jamroot.jam", """\
import assert ;

assert.result FooBarBaz : SPLIT_BY_CHARACTERS FooBarBaz : "" ;
assert.result FooBarBaz : SPLIT_BY_CHARACTERS FooBarBaz : x ;
assert.result FooBa Baz : SPLIT_BY_CHARACTERS FooBarBaz : r ;
assert.result FooBa Baz : SPLIT_BY_CHARACTERS FooBarBaz : rr ;
assert.result FooBa Baz : SPLIT_BY_CHARACTERS FooBarBaz : rrr ;
assert.result FooB rB z : SPLIT_BY_CHARACTERS FooBarBaz : a ;
assert.result FooB B z : SPLIT_BY_CHARACTERS FooBarBaz : ar ;
assert.result ooBarBaz : SPLIT_BY_CHARACTERS FooBarBaz : F ;
assert.result FooBarBa : SPLIT_BY_CHARACTERS FooBarBaz : z ;
assert.result ooBarBa : SPLIT_BY_CHARACTERS FooBarBaz : Fz ;
assert.result F B rB z : SPLIT_BY_CHARACTERS FooBarBaz : oa ;
assert.result Alib b : SPLIT_BY_CHARACTERS Alibaba : oa ;
assert.result libaba : SPLIT_BY_CHARACTERS Alibaba : oA ;
assert.result : SPLIT_BY_CHARACTERS FooBarBaz : FooBarBaz ;
assert.result : SPLIT_BY_CHARACTERS FooBarBaz : FoBarz ;

# Questionable results - should they return an empty string or an empty list?
assert.result : SPLIT_BY_CHARACTERS "" : "" ;
assert.result : SPLIT_BY_CHARACTERS "" : x ;
assert.result : SPLIT_BY_CHARACTERS "" : r ;
assert.result : SPLIT_BY_CHARACTERS "" : rr ;
assert.result : SPLIT_BY_CHARACTERS "" : rrr ;
assert.result : SPLIT_BY_CHARACTERS "" : oa ;
""")
    t.run_build_system()
    t.cleanup()

test_invalid("", "missing argument string")
test_invalid("Foo", "missing argument delimiters")
test_invalid(": Bar", "missing argument string")
test_invalid("a : b : c", "extra argument c")
test_invalid("a b : c", "extra argument b")
test_invalid("a : b c", "extra argument c")
test_valid()
