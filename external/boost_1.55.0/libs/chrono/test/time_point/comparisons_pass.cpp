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

#include <boost/chrono/chrono.hpp>
#include <boost/detail/lightweight_test.hpp>
#ifdef BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif


int main()
{
  typedef boost::chrono::system_clock Clock;
  typedef boost::chrono::milliseconds Duration1;
  typedef boost::chrono::microseconds Duration2;
  typedef boost::chrono::time_point<Clock, Duration1> T1;
  typedef boost::chrono::time_point<Clock, Duration2> T2;

  {
    T1 t1(Duration1(3));
    T1 t2(Duration1(3));
    BOOST_TEST( (t1 == t2));
    BOOST_TEST(! (t1 != t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T1 t2(Duration1(3));
    BOOST_CONSTEXPR_ASSERT( (t1 == t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 != t2));
  }
  {
    T1 t1(Duration1(3));
    T1 t2(Duration1(4));
    BOOST_TEST(! (t1 == t2));
    BOOST_TEST( (t1 != t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T1 t2(Duration1(4));
    BOOST_CONSTEXPR_ASSERT(! (t1 == t2));
    BOOST_CONSTEXPR_ASSERT( (t1 != t2));
  }
  {
    T1 t1(Duration1(3));
    T2 t2(Duration2(3000));
    BOOST_TEST( (t1 == t2));
    BOOST_TEST(! (t1 != t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T2 t2(Duration2(3000));
    BOOST_CONSTEXPR_ASSERT( (t1 == t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 != t2));
  }
  {
    T1 t1(Duration1(3));
    T2 t2(Duration2(3001));
    BOOST_TEST(! (t1 == t2));
    BOOST_TEST( (t1 != t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T2 t2(Duration2(3001));
    BOOST_CONSTEXPR_ASSERT(! (t1 == t2));
    BOOST_CONSTEXPR_ASSERT( (t1 != t2));
  }
  {
    T1 t1(Duration1(3));
    T1 t2(Duration1(3));
    BOOST_TEST(! (t1 < t2));
    BOOST_TEST(! (t1 > t2));
    BOOST_TEST( (t1 <= t2));
    BOOST_TEST( (t1 >= t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T1 t2(Duration1(3));
    BOOST_CONSTEXPR_ASSERT(! (t1 < t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 > t2));
    BOOST_CONSTEXPR_ASSERT( (t1 <= t2));
    BOOST_CONSTEXPR_ASSERT( (t1 >= t2));
  }
  {
    T1 t1(Duration1(3));
    T1 t2(Duration1(4));
    BOOST_TEST( (t1 < t2));
    BOOST_TEST(! (t1 > t2));
    BOOST_TEST( (t1 <= t2));
    BOOST_TEST(! (t1 >= t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T1 t2(Duration1(4));
    BOOST_CONSTEXPR_ASSERT( (t1 < t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 > t2));
    BOOST_CONSTEXPR_ASSERT( (t1 <= t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 >= t2));
  }
  {
    T1 t1(Duration1(3));
    T2 t2(Duration2(3000));
    BOOST_TEST(! (t1 < t2));
    BOOST_TEST(! (t1 > t2));
    BOOST_TEST( (t1 <= t2));
    BOOST_TEST( (t1 >= t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T2 t2(Duration2(3000));
    BOOST_CONSTEXPR_ASSERT(! (t1 < t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 > t2));
    BOOST_CONSTEXPR_ASSERT( (t1 <= t2));
    BOOST_CONSTEXPR_ASSERT( (t1 >= t2));
  }
  {
    T1 t1(Duration1(3));
    T2 t2(Duration2(3001));
    BOOST_TEST( (t1 < t2));
    BOOST_TEST(! (t1 > t2));
    BOOST_TEST( (t1 <= t2));
    BOOST_TEST(! (t1 >= t2));
  }
  {
    BOOST_CONSTEXPR T1 t1(Duration1(3));
    BOOST_CONSTEXPR T2 t2(Duration2(3001));
    BOOST_CONSTEXPR_ASSERT( (t1 < t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 > t2));
    BOOST_CONSTEXPR_ASSERT( (t1 <= t2));
    BOOST_CONSTEXPR_ASSERT(! (t1 >= t2));
  }

  return boost::report_errors();
}
