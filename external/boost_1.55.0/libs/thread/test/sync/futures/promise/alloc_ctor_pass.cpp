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

//   promise(allocator_arg_t, const Allocator& a);

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#include "../test_allocator.hpp"

int main()
{
  BOOST_TEST(test_alloc_base::count == 0);
  {
      boost::promise<int> p(boost::allocator_arg, test_allocator<int>());
      BOOST_TEST(test_alloc_base::count == 1);
      boost::future<int> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);
  {
      boost::promise<int&> p(boost::allocator_arg, test_allocator<int>());
      BOOST_TEST(test_alloc_base::count == 1);
      boost::future<int&> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);
  {
      boost::promise<void> p(boost::allocator_arg, test_allocator<void>());
      BOOST_TEST(test_alloc_base::count == 1);
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);


  return boost::report_errors();
}

#else
int main()
{
  return boost::report_errors();
}
#endif


