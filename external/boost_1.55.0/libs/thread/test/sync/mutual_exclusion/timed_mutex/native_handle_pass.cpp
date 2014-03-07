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

// <boost/thread/timed_mutex.hpp>

// class timed_mutex;

// typedef pthread_timed_mutex_t* native_handle_type;
// native_handle_type native_handle();

#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>


int main()
{
#if defined BOOST_THREAD_DEFINES_TIMED_MUTEX_NATIVE_HANDLE
  boost::timed_mutex m;
  boost::timed_mutex::native_handle_type h = m.native_handle();
  BOOST_TEST(h);
#else
#error "Test not applicable: BOOST_THREAD_DEFINES_TIMED_MUTEX_NATIVE_HANDLE not defined for this platform as not supported"
#endif
  return boost::report_errors();
}

