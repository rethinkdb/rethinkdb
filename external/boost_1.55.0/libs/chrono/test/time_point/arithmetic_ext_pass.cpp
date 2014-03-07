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

#define BOOST_CHRONO_EXTENSIONS
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
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    t += 2;
    BOOST_TEST(t.time_since_epoch() == Duration(5));
  }
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    t++;
    BOOST_TEST(t.time_since_epoch() == Duration(4));
  }
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    ++t;
    BOOST_TEST(t.time_since_epoch() == Duration(4));
  }
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    t -= 2;
    BOOST_TEST(t.time_since_epoch() == Duration(1));
  }
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    t--;
    BOOST_TEST(t.time_since_epoch() == Duration(2));
  }
  {
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    boost::chrono::time_point<Clock, Duration> t(Duration(3));
    --t;
    BOOST_TEST(t.time_since_epoch() == Duration(2));
  }

  return boost::report_errors();
}
