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

// ToDuration shall be an instantiation of duration.

#include <boost/chrono/chrono.hpp>

void test()
{
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::time_point<Clock, boost::chrono::milliseconds> FromTimePoint;
    typedef boost::chrono::time_point<Clock, boost::chrono::minutes> ToTimePoint;
    boost::chrono::time_point_cast<ToTimePoint>(FromTimePoint(boost::chrono::milliseconds(3)));
}
