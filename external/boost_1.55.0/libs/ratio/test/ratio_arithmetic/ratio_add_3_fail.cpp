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

template <typename T, typename R>
struct S {
    T val;
};

boost::intmax_t func(S<int, boost::ratio<5,6> > const& s) {
    return s.val*3;    
}


boost::intmax_t test() {
    return func(
            S<int, boost::ratio_add<
                boost::ratio<1,2>,
                boost::ratio<1,3> 
            >
            //~ ::type 
            >() 
            );
}

