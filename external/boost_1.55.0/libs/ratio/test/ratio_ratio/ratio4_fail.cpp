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

//#define BOOST_RATIO_EXTENSIONS

#include <boost/ratio/ratio.hpp>

boost::intmax_t func(boost::ratio<5,6> const& s) {
    return s.num;    
}

boost::intmax_t test() {
    return func(boost::ratio<10,12>());
}

