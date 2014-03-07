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

#include <boost/ratio/mpl/less.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R1;
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, -1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1, BOOST_RATIO_INTMAX_T_MAX> R1;
    typedef boost::ratio<1, -BOOST_RATIO_INTMAX_T_MAX> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 0x7FFFFFFFFFFFFFFELL> R1;
    typedef boost::ratio<0x7FFFFFFFFFFFFFFDLL, 0x7FFFFFFFFFFFFFFCLL> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<0x7FFFFFFFFFFFFFFDLL, 0x7FFFFFFFFFFFFFFCLL> R1;
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 0x7FFFFFFFFFFFFFFELL> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<-0x7FFFFFFFFFFFFFFDLL, 0x7FFFFFFFFFFFFFFCLL> R1;
    typedef boost::ratio<-BOOST_RATIO_INTMAX_T_MAX, 0x7FFFFFFFFFFFFFFELL> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 0x7FFFFFFFFFFFFFFELL> R1;
    typedef boost::ratio<0x7FFFFFFFFFFFFFFELL, 0x7FFFFFFFFFFFFFFDLL> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<641981, 1339063> R1;
    typedef boost::ratio<1291640, 2694141LL> R2;
    BOOST_RATIO_STATIC_ASSERT((!boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
    {
    typedef boost::ratio<1291640, 2694141LL> R1;
    typedef boost::ratio<641981, 1339063> R2;
    BOOST_RATIO_STATIC_ASSERT((boost::mpl::less<R1, R2>::value), NOTHING, ());
    }
}
