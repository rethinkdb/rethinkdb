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

// void promise::set_value_at_thread_exit(const R& r);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
void func(boost::promise<int> p)
#else
boost::promise<int> p;
void func()
#endif
{
  const int i = 5;
  p.set_value_at_thread_exit(i);
}

int main()
{
  {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::promise<int> p;
    boost::future<int> f = p.get_future();
    boost::thread(func, boost::move(p)).detach();
#else
    boost::future<int> f = p.get_future();
    boost::thread(func).detach();
#endif
    try
    {
      BOOST_TEST(f.get() == 5);
    }
    catch (...)
    {
      BOOST_TEST(false);
    }
  }
  {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#else
    boost::promise<int> p2;
    boost::future<int> f = p2.get_future();
    p = boost::move(p2);
    boost::thread(func).detach();
    BOOST_TEST(f.get() == 5);
#endif
  }
  return boost::report_errors();
}

