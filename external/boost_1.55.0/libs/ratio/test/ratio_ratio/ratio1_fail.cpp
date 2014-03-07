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

// test ratio:  The template argument D mus not be zero

#include <boost/ratio/ratio.hpp>
#include <boost/cstdint.hpp>

void test()
{
    const boost::intmax_t t1 = boost::ratio<1, 0>::num;
    (void)t1;
}
