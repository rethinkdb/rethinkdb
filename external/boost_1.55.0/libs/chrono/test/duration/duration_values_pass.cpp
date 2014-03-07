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
#if defined BOOST_NO_CXX11_NUMERIC_LIMITS || defined BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif

template <class D>
void check_max()
{
    typedef typename D::rep Rep;
    Rep max_rep = (boost::chrono::duration_values<Rep>::max)();
    BOOST_TEST((D::max)().count() == max_rep);
    {
      typedef typename D::rep Rep;
      BOOST_CHRONO_LIB_CONSTEXPR Rep max_rep = (boost::chrono::duration_values<Rep>::max)();
      BOOST_CONSTEXPR_ASSERT((D::max)().count() == max_rep);
    }
}

template <class D>
void check_min()
{
    typedef typename D::rep Rep;
    Rep min_rep = (boost::chrono::duration_values<Rep>::min)();
    BOOST_TEST((D::min)().count() == min_rep);
    {
      typedef typename D::rep Rep;
      BOOST_CHRONO_LIB_CONSTEXPR Rep min_rep = (boost::chrono::duration_values<Rep>::min)();
      BOOST_CONSTEXPR_ASSERT((D::min)().count() == min_rep);

    }
}

template <class D>
void check_zero()
{
    typedef typename D::rep Rep;
    Rep zero_rep = boost::chrono::duration_values<Rep>::zero();
    BOOST_TEST(D::zero().count() == zero_rep);
    {
      typedef typename D::rep Rep;
      BOOST_CONSTEXPR Rep zero_rep = boost::chrono::duration_values<Rep>::zero();
      BOOST_CONSTEXPR_ASSERT(D::zero().count() == zero_rep);

    }
}


int main()
{
    check_max<boost::chrono::duration<int> >();
    check_max<boost::chrono::duration<Rep> >();
    check_min<boost::chrono::duration<int> >();
    check_min<boost::chrono::duration<Rep> >();
    check_zero<boost::chrono::duration<int> >();
    check_zero<boost::chrono::duration<Rep> >();
    return boost::report_errors();
}
