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

// <future>

// class promise<R>

// promise& operator=(promise&& rhs);

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#include "../test_allocator.hpp"
#endif

boost::mutex m0;
boost::mutex m1;

int main()
{
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
  BOOST_TEST(test_alloc_base::count == 0);
  {
    boost::promise<int> p0(boost::allocator_arg, test_allocator<int>());
    boost::promise<int> p(boost::allocator_arg, test_allocator<int>());
    BOOST_TEST(test_alloc_base::count == 2);
    p = boost::move(p0);
    BOOST_TEST(test_alloc_base::count == 1);
    boost::future<int> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(test_alloc_base::count == 1);
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
    BOOST_TEST(test_alloc_base::count == 1);
  }
  BOOST_TEST(test_alloc_base::count == 0);

  {
    boost::promise<int&> p0(boost::allocator_arg, test_allocator<int>());
    boost::promise<int&> p(boost::allocator_arg, test_allocator<int>());
    BOOST_TEST(test_alloc_base::count == 2);
    p = boost::move(p0);
    BOOST_TEST(test_alloc_base::count == 1);
    boost::future<int&> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(test_alloc_base::count == 1);
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
    BOOST_TEST(test_alloc_base::count == 1);
  }
  BOOST_TEST(test_alloc_base::count == 0);
  {
    boost::promise<void> p0(boost::allocator_arg, test_allocator<void>());
    boost::promise<void> p(boost::allocator_arg, test_allocator<void>());
    BOOST_TEST(test_alloc_base::count == 2);
    p = boost::move(p0);
    BOOST_TEST(test_alloc_base::count == 1);
    boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(test_alloc_base::count == 1);
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
    BOOST_TEST(test_alloc_base::count == 1);
  }
  BOOST_TEST(test_alloc_base::count == 0);

#endif
  {
    boost::promise<int> p0;
    boost::promise<int> p;
    p = boost::move(p0);
    boost::future<int> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
  }

  {
    boost::promise<int&> p0;
    boost::promise<int&> p;
    p = boost::move(p0);
    boost::future<int&> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
  }
  {
    boost::promise<void> p0;
    boost::promise<void> p;
    p = boost::move(p0);
    boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(f.valid());
    try
    {
      f = BOOST_THREAD_MAKE_RV_REF(p0.get_future());
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
  }

  return boost::report_errors();

}

