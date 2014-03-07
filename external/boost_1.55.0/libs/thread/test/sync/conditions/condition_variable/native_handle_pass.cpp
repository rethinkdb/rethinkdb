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

// <boost/thread/condition_variable>

// class condition_variable;

// condition_variable(const condition_variable&) = delete;

#include <boost/thread/condition_variable.hpp>

#include <boost/static_assert.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
#if defined BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE
  //BOOST_STATIC_ASSERT((boost::is_same<boost::condition_variable::native_handle_type, pthread_cond_t*>::value));
  boost::condition_variable cv;
  boost::condition_variable::native_handle_type h = cv.native_handle();
  BOOST_TEST(h != 0);
#else
#error "Test not applicable: BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE not defined for this platform as not supported"
#endif
  return boost::report_errors();
}

