#!/usr/bin/python
#
# Copyright (c) 2008 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test that the common.copy rule set the modification date of the new file to
# the current time.

import BoostBuild

tester = BoostBuild.Tester(use_test_config=False)

tester.write("test1.cpp", """\
template<bool, int M, class Next>
struct time_waster {
    typedef typename time_waster<true, M-1, time_waster>::type type1;
    typedef typename time_waster<false, M-1, time_waster>::type type2;
    typedef void type;
};
template<bool B, class Next>
struct time_waster<B, 0, Next> {
    typedef void type;
};
typedef time_waster<true, 10, void>::type type;
int f() { return 0; }
""")

tester.write("test2.cpp", """\
template<bool, int M, class Next>
struct time_waster {
    typedef typename time_waster<true, M-1, time_waster>::type type1;
    typedef typename time_waster<false, M-1, time_waster>::type type2;
    typedef void type;
};
template<bool B, class Next>
struct time_waster<B, 0, Next> {
    typedef void type;
};
typedef time_waster<true, 10, void>::type type;
int g() { return 0; }
""")

tester.write("jamroot.jam", """\
obj test2 : test2.cpp ;
obj test1 : test1.cpp : <dependency>test2 ;
install test2i : test2 : <dependency>test1 ;
""")

tester.run_build_system()
tester.expect_addition("bin/$toolset/debug/test2.obj")
tester.expect_addition("bin/$toolset/debug/test1.obj")
tester.expect_addition("test2i/test2.obj")
tester.expect_nothing_more()

test2src = tester.read("test2i/test2.obj")
test2dest = tester.read("bin/$toolset/debug/test2.obj")
if test2src != test2dest:
    BoostBuild.annotation("failure", "The object file was not copied "
        "correctly")
    tester.fail_test(1)

tester.run_build_system(["-d1"])
tester.expect_output_lines("common.copy*", False)
tester.expect_nothing_more()

tester.cleanup()
