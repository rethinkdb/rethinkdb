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

// test mpl::minus

#define BOOST_RATIO_EXTENSIONS

#include <boost/ratio/mpl/minus.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{

    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 0 && R::den == 1, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<-1, 2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -3 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, -2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -3 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<-1, 1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 3 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<1, -1> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 3 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<56987354, 467584654> R1;
    typedef boost::ratio<544668, 22145> R2;
    typedef boost::mpl::minus<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -126708206685271LL && R::den == 5177331081415LL, NOTHING, ());
    }
}
