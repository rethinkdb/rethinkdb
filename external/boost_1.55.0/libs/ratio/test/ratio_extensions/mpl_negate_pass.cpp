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

// test mpl::negate

#define BOOST_RATIO_EXTENSIONS

#include <boost/ratio/mpl/negate.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{

    {
    typedef boost::ratio<0> R1;
    typedef boost::mpl::negate<R1> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 0 && R::den == 1, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::mpl::negate<R1> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -1 && R::den == 1, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::mpl::negate<R1> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<-1, 2> R1;
    typedef boost::mpl::negate<R1> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, -2> R1;
    typedef boost::mpl::negate<R1> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 1 && R::den == 2, NOTHING, ());
    }
}
