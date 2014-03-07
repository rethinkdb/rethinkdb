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
// Test default template arg:

// template <class Rep, class Period = ratio<1>>
// class duration;

#include <boost/chrono/duration.hpp>
#include <boost/type_traits.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

BOOST_CHRONO_STATIC_ASSERT((boost::is_same<
    boost::chrono::duration<int, boost::ratio<1> >,
    boost::chrono::duration<int>
>::value), NOTHING, ());
