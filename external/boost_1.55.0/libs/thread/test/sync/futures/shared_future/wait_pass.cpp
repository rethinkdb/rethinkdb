//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class shared_future<R>

// template <class Rep, class Period>
// void wait() const;

//#define BOOST_THREAD_VERSION 3
#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID
#include <boost/thread/detail/log.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

typedef boost::chrono::milliseconds ms;

namespace boost
{
  template <typename OStream>
  OStream& operator<<(OStream& os , boost::future_status st )
  {
    os << int(st) << " ";
    return os;
  }
  template <typename T>
  struct wrap
  {
    wrap(T const& v) :
      value(v)
    {
    }
    T value;

  };

  template <typename T>
  exception_ptr make_exception_ptr(T v)
  {
    return copy_exception(wrap<T> (v));
  }
}

void func1(boost::promise<int> p)
{
  boost::this_thread::sleep_for(ms(500));
  p.set_value(3);
}

int j = 0;

void func3(boost::promise<int&> p)
{
  boost::this_thread::sleep_for(ms(500));
  j = 5;
  p.set_value(j);
}

void func5(boost::promise<void> p)
{
  boost::this_thread::sleep_for(ms(500));
  p.set_value();
}

int main()
{
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    typedef boost::chrono::high_resolution_clock Clock;
    {
      typedef int T;
      boost::promise<T> p;
      boost::shared_future<T> f((p.get_future()));
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func1, boost::move(p)).detach();
#else
      func1(boost::move(p));
#endif
      BOOST_TEST(f.valid());
      f.wait();
      BOOST_TEST(f.valid());
      Clock::time_point t0 = Clock::now();
      f.wait();
      Clock::time_point t1 = Clock::now();
      BOOST_TEST(f.valid());
      BOOST_TEST(t1 - t0 < ms(50));
    }
    {
      typedef int& T;
      boost::promise<T> p;
      boost::shared_future<T> f((p.get_future()));
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func3, boost::move(p)).detach();
#else
      func3(boost::move(p));
#endif
      BOOST_TEST(f.valid());
      f.wait();
      BOOST_TEST(f.valid());
      Clock::time_point t0 = Clock::now();
      f.wait();
      Clock::time_point t1 = Clock::now();
      BOOST_TEST(f.valid());
      BOOST_TEST(t1 - t0 < ms(50));
    }
    {
      typedef void T;
      boost::promise<T> p;
      boost::shared_future<T> f((p.get_future()));
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func5, boost::move(p)).detach();
#else
      func5(boost::move(p));
#endif
      BOOST_TEST(f.valid());
      f.wait();
      BOOST_TEST(f.valid());
      Clock::time_point t0 = Clock::now();
      f.wait();
      Clock::time_point t1 = Clock::now();
      BOOST_TEST(f.valid());
      BOOST_TEST(t1 - t0 < ms(50));
    }
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif
