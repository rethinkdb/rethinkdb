//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class promise<R>

// void promise::set_exception_at_thread_exit(exception_ptr p);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>

namespace boost
{
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

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
void func(boost::promise<int> p)
#else
boost::promise<int> p;
void func()
#endif
{
  //p.set_exception(boost::make_exception_ptr(3));
  p.set_exception_at_thread_exit(boost::make_exception_ptr(3));
}

int main()
{
  {
    typedef int T;
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    boost::thread(func, boost::move(p)).detach();
#else
    boost::future<T> f = p.get_future();
    boost::thread(func).detach();
#endif
    try
    {
      f.get();
      BOOST_TEST(false);
    }
    catch (boost::wrap<int> i)
    {
      BOOST_TEST(i.value == 3);
    }
    catch (...)
    {
      BOOST_TEST(false);
    }
  }
  {
    typedef int T;
    boost::promise<T> p2;
    boost::future<T> f = p2.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::thread(func, boost::move(p2)).detach();
#else
    p = boost::move(p2);
    boost::thread(func).detach();
#endif
    try
    {
      f.get();
      BOOST_TEST(false);
    }
    catch (boost::wrap<int> i)
    {
      BOOST_TEST(i.value == 3);
    }
    catch (...)
    {
      BOOST_TEST(false);
    }
  }
  return boost::report_errors();
}

