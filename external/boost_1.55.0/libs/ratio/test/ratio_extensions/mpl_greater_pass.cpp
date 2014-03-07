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

#define BOOST_RATIO_EXTENSIONS

#include <boost/ratio/mpl/greater.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R1;
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, -1> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R1;
    typedef boost::ratio<1, -BOOST_RATIO_INTMAX_T_MAX> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::greater<R1, R2>::value), NOTHING, ());
    }
}
