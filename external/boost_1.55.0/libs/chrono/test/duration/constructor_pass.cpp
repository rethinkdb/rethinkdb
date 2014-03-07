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


#include <boost/chrono/duration.hpp>
#include <boost/detail/lightweight_test.hpp>


#include "../rep.h"
#include <iostream>

#ifdef BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif

template <class D>
void
check_default()
{
  {
    D d;
    BOOST_TEST(d.count() == typename D::rep());
  }
  {
    BOOST_CONSTEXPR D d;
    BOOST_CONSTEXPR_ASSERT(d.count() == typename D::rep());
  }
}

template <class D, class R>
void
check_from_rep(R r)
{
  {
    D d(r);
    BOOST_TEST(d.count() == r);
  }
}

int main()
{
    // exact conversions allowed for integral reps
    {
        boost::chrono::milliseconds ms(1);
        boost::chrono::microseconds us = ms;
        BOOST_TEST(us.count() == 1000);
        {
          BOOST_CONSTEXPR boost::chrono::milliseconds ms(1);
          BOOST_CONSTEXPR boost::chrono::microseconds us = ms;
          BOOST_CONSTEXPR_ASSERT(us.count() == 1000);
        }
    }
    // inexact conversions allowed for floating point reps
    {
        boost::chrono::duration<double, boost::micro> us(1);
        boost::chrono::duration<double, boost::milli> ms = us;
        BOOST_TEST(ms.count() == 1./1000);
        {
          BOOST_CONSTEXPR boost::chrono::duration<double, boost::micro> us(1);
          BOOST_CONSTEXPR boost::chrono::duration<double, boost::milli> ms = us;
          BOOST_CONSTEXPR_ASSERT(ms.count() == 1./1000);
        }
    }
    // Convert int to float
    {
        boost::chrono::duration<int> i(3);
        boost::chrono::duration<double> d = i;
        BOOST_TEST(d.count() == 3);
        {
          BOOST_CONSTEXPR boost::chrono::duration<int> i(3);
          BOOST_CONSTEXPR boost::chrono::duration<double> d = i;
          BOOST_CONSTEXPR_ASSERT(d.count() == 3);
        }
    }
    // default constructor
    {
      check_default<boost::chrono::duration<Rep> >();
    }
    // constructor from rep
    {
        check_from_rep<boost::chrono::duration<int> >(5);
        {
          BOOST_CONSTEXPR boost::chrono::duration<int> d(5);
          BOOST_CONSTEXPR_ASSERT(d.count() == 5);
        }
        check_from_rep<boost::chrono::duration<int, boost::ratio<3, 2> > >(5);
        {
          BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<3, 2> > d(5);
          BOOST_CONSTEXPR_ASSERT(d.count() == 5);
        }
        check_from_rep<boost::chrono::duration<Rep, boost::ratio<3, 2> > >(Rep(3));
        {
          BOOST_CONSTEXPR boost::chrono::duration<Rep, boost::ratio<3, 2> > d(Rep(3));
          BOOST_CONSTEXPR_ASSERT(d.count() == Rep(3));
        }
        check_from_rep<boost::chrono::duration<double, boost::ratio<2, 3> > >(5.5);
        {
          BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<3, 2> > d(5.5);
          BOOST_CONSTEXPR_ASSERT(d.count() == 5.5);
        }


    }
    // constructor from other rep
    {
        boost::chrono::duration<double> d(5);
        BOOST_TEST(d.count() == 5);
        {
          BOOST_CONSTEXPR boost::chrono::duration<double> d(5);
          BOOST_CONSTEXPR_ASSERT(d.count() == 5);
        }
    }

    return boost::report_errors();
}
