#!/usr/bin/python

# Copyright (C) 2013 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import MockToolset

t = BoostBuild.Tester(arguments=['toolset=mock', '--ignore-site-config', '--user-config='], pass_toolset=0)

MockToolset.create(t)

# Build from source
t.write("zlib/zlib.h", 'zlib')
t.write("zlib/deflate.c", 'deflate')

t.write("Jamroot.jam", """
path-constant here : . ;
using zlib : : <source>$(here)/zlib ;
alias zlib : /zlib//zlib : : <link>static <link>shared ;
""")

MockToolset.set_expected(t, '''
source_file('deflate.c', 'deflate')
action('-c -x c -I./zlib -o $deflate.o $deflate.c')
action('--dll $deflate.o -o $deflate.so')
action('--archive $deflate.o -o $deflate.a')
''')

t.run_build_system()
t.expect_addition('bin/standalone/zlib/mock/debug/z.dll')
t.expect_addition('bin/standalone/zlib/mock/debug/link-static/z.lib')

t.rm('zlib')

# Generic definitions that aren't configuration specific
common_stuff = '''
source_file('test.cpp', 'test.cpp')
source_file('main.cpp', 'int main() {}')
source_file('zlib.h.cpp', '#include <zlib.h>')
action('-c -x c++ $main.cpp -o $main.o')
'''
t.write('test.cpp', 'test.cpp')

# Default initialization - static library
t.rm('bin')
t.write("Jamroot.jam", """
path-constant here : . ;
using zlib ;
exe test : test.cpp /zlib//zlib : : <link>static <link>shared ;
""")

MockToolset.set_expected(t, common_stuff + '''
action('$main.o --static-lib=z -o $config.exe')
action('-c -x c++ $zlib.h.cpp -o $zlib.h.o')
action('-c -x c++ $test.cpp -o $test.o')
action('$test.o --static-lib=z -o $test')
''')
t.run_build_system()
t.expect_addition('bin/mock/debug/test.exe')
t.expect_addition('bin/mock/debug/link-static/test.exe')

# Default initialization - shared library
t.rm('bin')
t.write("Jamroot.jam", """
path-constant here : . ;
using zlib ;
exe test : test.cpp /zlib//zlib : : <link>static <link>shared ;
""")

MockToolset.set_expected(t, common_stuff + '''
action('$main.o --shared-lib=z -o $config.exe')
action('-c -x c++ $zlib.h.cpp -o $zlib.h.o')
action('-c -x c++ $test.cpp -o $test.o')
action('$test.o --shared-lib=z -o $test')
''')
t.run_build_system()
t.expect_addition('bin/mock/debug/test.exe')
t.expect_addition('bin/mock/debug/link-static/test.exe')

# Initialization in explicit location - static library
t.rm('bin')
t.write("Jamroot.jam", """
path-constant here : . ;
using zlib : : <name>myzlib <include>$(here)/zlib <search>$(here)/zlib ;
exe test : test.cpp /zlib//zlib : : <link>static <link>shared ;
""")

t.write('zlib/zlib.h', 'zlib')

MockToolset.set_expected(t, common_stuff + '''
action('$main.o -L./zlib --static-lib=myzlib -o $config.exe')
action('-c -x c++ $test.cpp -I./zlib -o $test.o')
action('$test.o -L./zlib --static-lib=myzlib -o $test')
''')
t.run_build_system()
t.expect_addition('bin/mock/debug/test.exe')
t.expect_addition('bin/mock/debug/link-static/test.exe')

# Initialization in explicit location - shared library
t.rm('bin')
t.write("Jamroot.jam", """
path-constant here : . ;
using zlib : : <name>myzlib <include>$(here)/zlib <search>$(here)/zlib ;
exe test : test.cpp /zlib//zlib : : <link>static <link>shared ;
""")

MockToolset.set_expected(t, common_stuff + '''
action('$main.o -L./zlib --shared-lib=myzlib -o $config.exe')
action('-c -x c++ $test.cpp -I./zlib -o $test.o')
action('$test.o -L./zlib --shared-lib=myzlib -o $test')
''')
t.run_build_system()
t.expect_addition('bin/mock/debug/test.exe')
t.expect_addition('bin/mock/debug/link-static/test.exe')

t.cleanup()
