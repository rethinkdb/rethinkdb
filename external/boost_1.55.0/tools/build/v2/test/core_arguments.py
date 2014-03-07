#!/usr/bin/python

# Copyright 2001 Dave Abrahams
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild


def simple_args(start, finish):
    return " : ".join("%d" % x for x in xrange(start, finish + 1))


def test(t, type, input, output, status=0):
    code = ["include echo_args.jam ; echo_%s" % type]
    if input: code.append(input)
    code.append(";")
    t.write("file.jam", " ".join(code))
    t.run_build_system(["-ffile.jam"], status=status)
    t.expect_output_lines(output);


def test_args(t, *args, **kwargs):
    test(t, "args", *args, **kwargs)


def test_varargs(t, *args, **kwargs):
    test(t, "varargs", *args, **kwargs)


t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

t.write("echo_args.jam", """\
NOCARE all ;

rule echo_args ( a b ? c ? : d + : e * )
{
    ECHO a= $(a) b= $(b) c= $(c) ":" d= $(d) ":" e= $(e) ;
}

rule echo_varargs ( a b ? c ? : d + : e * : * )
{
    ECHO a= $(a) b= $(b) c= $(c) ":" d= $(d) ":" e= $(e)
        ": rest= "$(4[1]) $(4[2-])
        ": "$(5[1])  $(5[2-])  ": "$(6[1])  $(6[2-])  ": "$(7[1])  $(7[2-])
        ": "$(8[1])  $(8[2-])  ": "$(9[1])  $(9[2-])  ": "$(10[1]) $(10[2-])
        ": "$(11[1]) $(11[2-]) ": "$(12[1]) $(12[2-]) ": "$(13[1]) $(13[2-])
        ": "$(14[1]) $(14[2-]) ": "$(15[1]) $(15[2-]) ": "$(16[1]) $(16[2-])
        ": "$(17[1]) $(17[2-]) ": "$(18[1]) $(18[2-]) ": "$(19[1]) $(19[2-])
        ": "$(20[1]) $(20[2-]) ": "$(21[1]) $(21[2-]) ": "$(22[1]) $(22[2-])
        ": "$(23[1]) $(23[2-]) ": "$(24[1]) $(24[2-]) ": "$(25[1]) $(25[2-]) ;
}
""")

test_args(t, "", "* missing argument a", status=1)
test_args(t, "1 2 : 3 : 4 : 5", "* extra argument 5", status=1)
test_args(t, "a b c1 c2 : d", "* extra argument c2", status=1)

# Check modifier '?'
test_args(t, "1 2 3 : 4", "a= 1 b= 2 c= 3 : d= 4 : e=")
test_args(t, "1 2 : 3", "a= 1 b= 2 c= : d= 3 : e=")
test_args(t, "1 2 : 3", "a= 1 b= 2 c= : d= 3 : e=")
test_args(t, "1 : 2", "a= 1 b= c= : d= 2 : e=")

# Check modifier '+'
test_args(t, "1", "* missing argument d", status=1)
test_args(t, "1 : 2 3", "a= 1 b= c= : d= 2 3 : e=")
test_args(t, "1 : 2 3 4", "a= 1 b= c= : d= 2 3 4 : e=")

# Check modifier '*'
test_args(t, "1 : 2 : 3", "a= 1 b= c= : d= 2 : e= 3")
test_args(t, "1 : 2 : 3 4", "a= 1 b= c= : d= 2 : e= 3 4")
test_args(t, "1 : 2 : 3 4 5", "a= 1 b= c= : d= 2 : e= 3 4 5")

# Check varargs
test_varargs(t, "1 : 2 : 3 4 5", "a= 1 b= c= : d= 2 : e= 3 4 5")
test_varargs(t, "1 : 2 : 3 4 5 : 6", "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= 6")
test_varargs(t, "1 : 2 : 3 4 5 : 6 7",
    "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= 6 7")
test_varargs(t, "1 : 2 : 3 4 5 : 6 7 : 8",
    "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= 6 7 : 8")
test_varargs(t, "1 : 2 : 3 4 5 : 6 7 : 8 : 9",
    "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= 6 7 : 8 : 9")
test_varargs(t, "1 : 2 : 3 4 5 : 6 7 : 8 : 9 : 10 : 11 : 12 : 13 : 14 : 15 : "
    "16 : 17 : 18 : 19a 19b", "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= 6 7 : 8 : "
    "9 : 10 : 11 : 12 : 13 : 14 : 15 : 16 : 17 : 18 : 19a 19b")
test_varargs(t, "1 : 2 : 3 4 5 : 6 7 : 8 : 9 : 10 : 11 : 12 : 13 : 14 : 15 : "
    "16 : 17 : 18 : 19a 19b 19c : 20", "a= 1 b= c= : d= 2 : e= 3 4 5 : rest= "
    "6 7 : 8 : 9 : 10 : 11 : 12 : 13 : 14 : 15 : 16 : 17 : 18 : 19a 19b 19c : "
    "20")

# Check varargs upper limit
expected = "a= 1 b= c= : d= 2 : e= 3 : rest= " + simple_args(4, 19)
test_varargs(t, simple_args(1, 19), expected)
test_varargs(t, simple_args(1, 19) + " 19b 19c 19d", expected + " 19b 19c 19d")
test_varargs(t, simple_args(1, 19) + " 19b 19c 19d : 20", expected + " 19b "
    "19c 19d")
test_varargs(t, simple_args(1, 20), expected)
test_varargs(t, simple_args(1, 50), expected)

t.cleanup()
