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

// test ratio_multiply

#include <boost/ratio/ratio.hpp>

typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
typedef boost::ratio<2,1> R2;
typedef boost::ratio_multiply<R1, R2>::type RT;
