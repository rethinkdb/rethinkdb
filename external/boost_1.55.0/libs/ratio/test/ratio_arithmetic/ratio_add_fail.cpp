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

#include <boost/ratio/ratio.hpp>
#include <boost/integer_traits.hpp>

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif


typedef boost::ratio<boost::integer_traits<boost::intmax_t>::const_max, 1> R1;
typedef boost::ratio<1, 1> R2;
typedef boost::ratio_add<R1, R2>::type RT;

BOOST_RATIO_STATIC_ASSERT(RT::num==boost::integer_traits<boost::intmax_t>::const_max+1, NOTHING, (RT));
BOOST_RATIO_STATIC_ASSERT(RT::den==1, NOTHING, (RT));
