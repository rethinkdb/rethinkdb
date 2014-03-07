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

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<boost::chrono::system_clock::duration,
                   boost::chrono::time_point<boost::chrono::system_clock>::duration>::value), NOTHING, ());
}
