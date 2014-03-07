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


template <typename R>
struct numerator;

template <boost::intmax_t N, boost::intmax_t D>
struct numerator<boost::ratio<N,D> > {
    static const boost::intmax_t value = N;    
};


BOOST_RATIO_STATIC_ASSERT((
        numerator<boost::ratio_add<boost::ratio<1,2>,boost::ratio<1,3> > >::value == 1)
        , NOTHING, ());
