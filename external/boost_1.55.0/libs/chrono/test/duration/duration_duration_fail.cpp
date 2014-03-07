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

// duration
// If a program instantiates duration with a duration type for the template
// argument Rep a diagnostic is required.

#include <boost/chrono/duration.hpp>

void test()
{
    typedef boost::chrono::duration<boost::chrono::milliseconds> D;
    D d;
}
