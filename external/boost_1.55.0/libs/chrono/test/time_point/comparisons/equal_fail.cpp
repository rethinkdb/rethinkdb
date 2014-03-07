//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

// time_points with different clocks should not compare

#include <boost/chrono/chrono.hpp>

#include "../../clock.h"

void test()
{
    typedef boost::chrono::system_clock Clock1;
    typedef Clock                     Clock2;
    typedef boost::chrono::milliseconds Duration1;
    typedef boost::chrono::microseconds Duration2;
    typedef boost::chrono::time_point<Clock1, Duration1> T1;
    typedef boost::chrono::time_point<Clock2, Duration2> T2;

    T1 t1(Duration1(3));
    T2 t2(Duration2(3000));
    t1 == t2;
}
