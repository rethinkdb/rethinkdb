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

// class packaged_task<R(ArgTypes...)>

// template <class Callable, class Alloc>
//   struct uses_allocator<packaged_task<Callable>, Alloc>
//      : true_type { };


#define BOOST_THREAD_VERSION 4
#if BOOST_THREAD_VERSION == 4
#define BOOST_THREAD_DETAIL_SIGNATURE double()
#else
#define BOOST_THREAD_DETAIL_SIGNATURE double
#endif

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#include "../test_allocator.hpp"

int main()
{

  BOOST_STATIC_ASSERT_MSG((boost::uses_allocator<boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE>, test_allocator<double> >::value), "");

  return boost::report_errors();
}

#else
int main()
{
  return boost::report_errors();
}
#endif


