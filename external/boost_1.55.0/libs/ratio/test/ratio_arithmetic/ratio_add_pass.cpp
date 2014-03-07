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

// test ratio_add

#include <boost/ratio/ratio.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

void test()
{
  {
  typedef boost::ratio<0> R1;
  typedef boost::ratio<0> R2;
  typedef boost::ratio_add<R1, R2> R;
  BOOST_RATIO_STATIC_ASSERT(R::num == 0 && R::den == 1, NOTHING, ());
  }
    {
    typedef boost::ratio<1, 1> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 2 && R::den == 1, NOTHING, ());
    typedef boost::ratio_add<R, R2> RR;
    BOOST_RATIO_STATIC_ASSERT(RR::num == 3 && RR::den == 1, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 3 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<-1, 2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, -2> R1;
    typedef boost::ratio<1, 1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<-1, 1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<1, 2> R1;
    typedef boost::ratio<1, -1> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == -1 && R::den == 2, NOTHING, ());
    }
    {
    typedef boost::ratio<56987354, 467584654> R1;
    typedef boost::ratio<544668, 22145> R2;
    typedef boost::ratio_add<R1, R2> R;
    BOOST_RATIO_STATIC_ASSERT(R::num == 127970191639601LL && R::den == 5177331081415LL, NOTHING, ());
    }
    {
    typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, 1> R1;
    typedef boost::ratio<-1, 1> R2;
    typedef boost::ratio_add<R1, R2>::type RT;
    }

}

boost::intmax_t func(boost::ratio<5,6> s) {
    return s.num;
}


boost::intmax_t test_conversion() {
    return func(
            boost::ratio_add<
                boost::ratio<1,2>,
                boost::ratio<1,3>
            >
            ()
            );
}

