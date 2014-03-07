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

template <class T>
void
test()
{
    BOOST_CHRONO_STATIC_ASSERT((boost::is_base_of<boost::is_floating_point<T>,
                                   boost::chrono::treat_as_floating_point<T> >::value), NOTHING, ());
}

struct A {};

void testall()
{
    test<int>();
    test<unsigned>();
    test<char>();
    test<bool>();
    test<float>();
    test<double>();
    test<long double>();
    test<A>();
}
