#!/usr/bin/python

# Copyright 2003 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# This tests correct handling of dependencies, specifically, on generated
# sources, and from generated sources.

import BoostBuild

import string

t = BoostBuild.Tester(pass_toolset=0)

t.write("core-dependency-helpers", """
rule hdrrule
{
   INCLUDES $(1) : $(2) ;
}
actions copy
{
   cp $(>) $(<)
}
""")

code = """include core-dependency-helpers ;
DEPENDS all : a ;
DEPENDS a : b ;

actions create-b
{
   echo '#include <foo.h>' > $(<) 
}
copy a : b ;
create-b b ;
HDRRULE on b foo.h bar.h = hdrrule ;
HDRSCAN on b foo.h bar.h = \"#include <(.*)>\" ;
"""

# This creates 'a' which depends on 'b', which is generated. The generated 'b'
# contains '#include <foo.h>' and no rules for foo.h are given. The system
# should error out on the first invocation.
t.run_build_system("-f-", stdin=code)
t.fail_test(string.find(t.stdout(), "...skipped a for lack of foo.h...") == -1)

t.rm('b')

# Now test that if target 'c' also depends on 'b', then it will not be built, as
# well.
t.run_build_system("-f-", stdin=code + " copy c : b ; DEPENDS c : b ; DEPENDS all : c ; ")
t.fail_test(string.find(t.stdout(), "...skipped c for lack of foo.h...") == -1)

t.rm('b')

# Now add a rule for creating foo.h.
code += """
actions create-foo
{
    echo // > $(<)
}
create-foo foo.h ;
"""
t.run_build_system("-f-", stdin=code)

# Run two times, adding explicit dependency from all to foo.h at the beginning
# and at the end, to make sure that foo.h is generated before 'a' in all cases.

def mk_correct_order_func(s1, s2):
    def correct_order(s):
        n1 = string.find(s, s1)
        n2 = string.find(s, s2)
        return ( n1 != -1 ) and ( n2 != -1 ) and ( n1 < n2 )
    return correct_order

correct_order = mk_correct_order_func("create-foo", "copy a")

t.rm(["a", "b", "foo.h"])
t.run_build_system("-d+2 -f-", stdin=code + " DEPENDS all : foo.h ;")
t.fail_test(not correct_order(t.stdout()))

t.rm(["a", "b", "foo.h"])
t.run_build_system("-d+2 -f-", stdin=" DEPENDS all : foo.h ; " + code)
t.fail_test(not correct_order(t.stdout()))

# Now foo.h exists. Test include from b -> foo.h -> bar.h -> biz.h. b and foo.h
# already have updating actions. 
t.rm(["a", "b"])
t.write("foo.h", "#include <bar.h>")
t.write("bar.h", "#include <biz.h>")
t.run_build_system("-d+2 -f-", stdin=code)
t.fail_test(string.find(t.stdout(), "...skipped a for lack of biz.h...") == -1)

# Add an action for biz.h.
code += """
actions create-biz
{
   echo // > $(<)
}
create-biz biz.h ;
"""

t.rm(["b"])
correct_order = mk_correct_order_func("create-biz", "copy a")
t.run_build_system("-d+2 -f-", stdin=code + " DEPENDS all : biz.h ;")
t.fail_test(not correct_order(t.stdout()))

t.rm(["a", "biz.h"])
t.run_build_system("-d+2 -f-", stdin=" DEPENDS all : biz.h ; " + code)
t.fail_test(not correct_order(t.stdout()))           

t.write("a", "")

code="""
DEPENDS all : main d ;

actions copy 
{
    cp $(>) $(<) ;
}

DEPENDS main : a ;
copy main : a ;

INCLUDES a : <1>c ;

NOCARE <1>c ;
SEARCH on <1>c = . ;

actions create-c 
{
    echo d > $(<)    
}

actions create-d
{
    echo // > $(<)
}

create-c <2>c ;
LOCATE on <2>c = . ;
create-d d ;

HDRSCAN on <1>c = (.*) ;
HDRRULE on <1>c = hdrrule ;

rule hdrrule 
{
    INCLUDES $(1) : d ;
}
"""

correct_order = mk_correct_order_func("create-d", "copy main")
t.run_build_system("-d2 -f-", stdin=code)
t.fail_test(not correct_order(t.stdout()))

t.cleanup()
